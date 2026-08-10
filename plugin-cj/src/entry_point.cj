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

func hotfixDebug(): Bool {
    return envSet("HOTFIX_DEBUG")
}

public class EntryPoint <: CHIRPluginBase {
    public init() {
        super("EntryPoint")
    }

    static init() {
        PluginManager.registerCHIRPlugin({=> EntryPoint()})
    }

    public override func run(pkg: Package): Bool {
        if (envSet("HOTFIX_NOOP")) {
            return true
        }
        let pluginContext = createPluginContext(pkg)
        let tomlFilter = formFilterByTomlDirective()

        let patcher = Patcher(pkg, pluginContext)
        let patchables = findPatchables(pkg, tomlFilter)
        for (patchable in patchables) {
            if (hotfixDebug()) {
                println("[hotfix] patchable: " + patchable.funcName.getQualifiedName())
            }
            patcher.patch(patchable)
        }
        patcher.genPatchClass()

        if (hotfixDebug()) {
            println("===== [hotfix] generated classes =====")
            for (c in pkg.classDefs) {
                if (c.srcCodeName.contains("$")) {
                    println(c.toString())
                }
            }
            println("===== [hotfix] generated global vars =====")
            for (v in pkg.globalVars) {
                if (v.srcCodeName.contains("$")) {
                    println(v.toString())
                }
            }
            println("===== [hotfix] patched functions =====")
            for (patchable in patchables) {
                println(patchable.fn.toString())
            }
        }
        return true
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
