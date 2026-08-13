/*
 * Hotfix plugin for the Cangjie compiler (cjc) — Cangjie port.
 *
 * Cangjie rewrite of the original C++ CHIR plugin (src/EntryPoint.cpp).
 * A CHIR plugin is the direct analogue of the C++ `MetaTransform<Package>`
 * plus `CHIR_PLUGIN(...)`: `run(pkg)` is invoked once per package during the
 * CHIR generation phase.
 *
 * NOTE: This expands `stdx.plugin.@CHIRPlugin` by hand (adding the
 * `CHIRPluginBase` super type, the `super(name)` init, and the static
 * `PluginManager.registerCHIRPlugin` registration) so the module only needs
 * the shipped `stdx.plugin.manager` package rather than the `stdx.plugin`
 * macro package. The behaviour is identical to annotating the class with
 * `@CHIRPlugin`.
 */
package hotfixplugin

import stdx.chir.*
import stdx.plugin.manager.*
import std.collection.*
import std.regex.*
import std.fs.*
import std.io.*
import std.env.*

func envSet(name: String): Bool {
    if (let Some(v) <- getVariable(name)) {
        return v != "" && v != "0"
    }
    return false
}

public class EntryPoint <: CHIRPluginBase {
    public init() {
        super("EntryPoint")
    }

    static init() {
        PluginManager.registerCHIRPlugin({=> EntryPoint()})
    }

    public override func run(pkg: Package): Bool {
        debug { =>
            println("Running Hotfix plugin for package " + pkg.name)
        }
        let pluginContext = createPluginContext(pkg)
        let tomlFilter = formFilterByTomlDirective()

        let patcher = Patcher(pkg, pluginContext)
        let patchables = findPatchables(pkg, tomlFilter)
        for (patchable in patchables) {
            debug { =>
                println("")
                println("")
                println("")
                println("Found patchable method: " + patchable.funcName.getQualifiedName() + "(" +
                    patchable.fn.identifierWithoutPrefix + ")")
            }
            patcher.patch(patchable)
        }
        patcher.genPatchClass()

        if (envSet("HOTFIX_STUB_TEST")) {
            let patcherStub = PatcherStub(pkg)
            for (patchable in patchables) {
                patcherStub.patch(patchable)
            }
        }

        debug { =>
            dumpSerializedPackage(pkg)
            println("Finished running Hotfix plugin")
            println("")
            println("")
            println("")
        }
        return true
    }
}

private func dumpSerializedPackage(pkg: Package): Unit {
    let serialized = serializePackage(pkg)
    let data = serialized[0]
    let size = serialized[1]
    let chunkSize: Int64 = 64 * 1024
    try (out = File("plugin" + pkg.name + ".chir", OpenMode.Write)) {
        var offset: Int64 = 0
        while (offset < size) {
            let currentSize = if (size - offset < chunkSize) { size - offset } else { chunkSize }
            let chunkStart = offset
            let chunk = Array<Byte>(currentSize, {i => unsafe { data.read(chunkStart + i) }})
            out.write(chunk)
            offset += currentSize
        }
    }
    unsafe {
        freeSerializedMemory()
    }
}

/**
 * Build the patchable name filter from the `[hotfix] app-patchable` directive of
 * the consumer's cjpm.toml (port of EntryPoint::formFilterByTomlDirective). A
 * minimal single-line TOML reader is used in place of the C++ toml++ library.
 */
func formFilterByTomlDirective(): ?Regex {
    let result = ArrayList<String>()
    if (let Some(text) <- readFileText("cjpm.toml")) {
        var inHotfix = false
        for (raw in text.split("\n")) {
            let line = raw.trimAscii()
            if (line.startsWith("[")) {
                inHotfix = line == "[hotfix]"
                continue
            }
            if (inHotfix && line.startsWith("app-patchable")) {
                for (s in extractQuotedStrings(line)) {
                    result.add(s)
                }
            }
        }
    }
    return formRegex(result.toArray())
}

/** Read a text file, returning None if it does not exist or cannot be read. */
private func readFileText(path: String): ?String {
    if (!exists(path)) {
        return None
    }
    var content: ?String = None
    try (f = File(path, OpenMode.Read)) {
        content = String.fromUtf8(readToEnd(f))
    } catch (_: Exception) {
        content = None
    }
    return content
}

/** Extract every double-quoted substring from a line (order preserved). */
private func extractQuotedStrings(line: String): Array<String> {
    let out = ArrayList<String>()
    var inQuote = false
    let cur = StringBuilder()
    for (c in line.toRuneArray()) {
        if (c == r'"') {
            if (inQuote) {
                out.add(cur.toString())
                cur.reset()
                inQuote = false
            } else {
                inQuote = true
            }
        } else if (inQuote) {
            cur.append(c)
        }
    }
    return out.toArray()
}
