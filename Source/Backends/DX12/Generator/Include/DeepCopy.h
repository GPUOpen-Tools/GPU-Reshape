#pragma once

// Std
#include <string>

/// Get the friendly deep copy name
/// \param name top typename
/// \return deep copy name
static std::string GetDeepCopyName(const std::string& name) {
    std::string contents;
    contents.reserve(name.length());

    // Capitalize next character?
    bool capitalize = true;

    // Consume
    for (size_t i = 0; i < name.length(); i++) {
        if (name[i] == '_') {
            capitalize = true;
            continue;
        }

        char ch = name[i];
        contents.push_back(capitalize ? std::toupper(ch) : std::tolower(ch));

        // If digit, capitalize next
        capitalize = isdigit(ch);
    }

    // Postfix
    return contents + "DeepCopy";
}
