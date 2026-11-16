#include "TypeFilterGenerator.h"

#include <iostream>

namespace HotfixPlugin {
std::optional<std::regex> formRegex(const std::vector<std::string>& filter)
{
    if (filter.empty()) {
        return std::nullopt; // no filter exists, nothing to form
    }
    std::string nameRegexp;
    if (filter.size() > 1) {
        nameRegexp += "(";
    }
    // the filter doesn't check whether there is correct package identifier, ok for prototyping.
    for (const auto& f : filter) {
        bool foundAsterisk = false;
        for (const char sym : f) {
            switch (sym) {
                case '.':
                    nameRegexp += "\\.";
                    break;
                case '*':
                    if (foundAsterisk) {
                        nameRegexp += ".+";
                        foundAsterisk = false;
                    } else {
                        nameRegexp += "[^.]+";
                        foundAsterisk = true;
                    }
                    // other cases such as *** lead to undefined filtering, ok for prototyping.
                    break;
                default:
                    nameRegexp += sym;
            }
        }
        nameRegexp += "|";
    }
    nameRegexp.erase(nameRegexp.length() - 1); // removing the last redundant '|' symbol
    if (filter.size() > 1) {
        nameRegexp += ")";
    }
#ifdef DEBUG
    std::cout << "Regexp by input filter: " << nameRegexp << std::endl;
#endif
    std::regex pattern{nameRegexp};
    return pattern;
}
}