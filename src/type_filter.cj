/*
 * Cangjie port of src/TypeFilterGenerator.cpp.
 *
 * Builds a regular expression from a sequence of filter glob strings taken from
 * the `[hotfix] app-patchable` directive of the consumer's cjpm.toml.
 *
 * Examples (mirrors the original C++ behaviour exactly, including that `**`
 * expands to `[^.]+.+` rather than `.+`):
 *   "pkg1"           -> pkg1
 *   "pkg1.Class1"    -> pkg1\.Class1
 *   "pkg2.*.Func1"   -> pkg2\.[^.]+\.Func1
 *   "pkg3.**.Class3" -> pkg3\.[^.]+.+\.Class3
 *
 * The compiled pattern is anchored with `^...$` so that `Regex.matches` (which
 * performs a search) reproduces C++ `std::regex_match` full-match semantics.
 */
package hotfixplugin

import std.regex.*

/**
 * Form a regular expression from the input sequence of filter strings.
 *
 * Returns `None` when the filter list is empty (no filter, nothing to form),
 * mirroring the C++ `std::optional<std::regex>` return.
 */
func formRegex(filter: Array<String>): ?Regex {
    if (filter.isEmpty()) {
        return None // no filter exists, nothing to form
    }

    var nameRegexp = ""
    if (filter.size > 1) {
        nameRegexp += "("
    }

    // The filter doesn't check whether there is a correct package identifier,
    // which is fine for prototyping (same caveat as the original).
    for (f in filter) {
        var foundAsterisk = false
        for (sym in f.toRuneArray()) {
            match (sym) {
                case r'.' => nameRegexp += "\\."
                case r'*' =>
                    if (foundAsterisk) {
                        nameRegexp += ".+"
                        foundAsterisk = false
                    } else {
                        nameRegexp += "[^.]+"
                        foundAsterisk = true
                    }
                    // Other cases such as *** lead to undefined filtering,
                    // which is fine for prototyping.
                case _ => nameRegexp += sym.toString()
            }
        }
        nameRegexp += "|"
    }
    // Remove the last redundant '|' symbol.
    nameRegexp = nameRegexp[..nameRegexp.size - 1]

    if (filter.size > 1) {
        nameRegexp += ")"
    }

    let nameRegexpBuf = nameRegexp
    debug { =>
        println("Regexp by input filter: " + nameRegexpBuf)
    }

    // Anchor to get full-match semantics equivalent to std::regex_match.
    return Regex("^" + nameRegexp + "$")
}

/**
 * Used for tests: returns true if `name` matches the regex formed from `filter`.
 * Mirrors the inline `matches` helper in the original TypeFilterGenerator.h.
 */
func matchesFilter(name: String, filter: Array<String>): Bool {
    if (let Some(pattern) <- formRegex(filter)) {
        return pattern.matches(name)
    }
    return false
}
