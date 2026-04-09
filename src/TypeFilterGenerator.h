#ifndef TOMLFILTERGENERATOR_H
#define TOMLFILTERGENERATOR_H

#include <regex>
#include <optional>

namespace HotfixPlugin {

/**
 * Form regular expression by input seq of filter strings.
 *
 * Examples:
 * @code
 * "pkg1"            -> ^pkg1$
 * "pkg1.Class1",    -> ^pkg2[.]Class1$
 * "pkg1.Func1",     -> ^pkg1[.]Func1$
 * "pkg2.*.Func1",   -> ^pkg2[.][^\.]+[.]Func1$
 * "pkg3.**.Class3"  -> ^pkg3[.].+[.]Class3$
 * @endcode
 */
std::optional<std::regex> formRegex(const std::vector<std::string>& filter);

/**
 * Used for tests.
 *
 * @param name input name
 * @param filter seq of filters containing conrete name or mask
 * @return true, if name matches to @code filter @endcode.
*/
inline bool matches(const std::string_view name, const std::vector<std::string>& filter)
{
    const auto pattern = formRegex(filter);
    return pattern.has_value() && std::regex_match(name.data(), pattern.value());
}
}
#endif //TOMLFILTERGENERATOR_H