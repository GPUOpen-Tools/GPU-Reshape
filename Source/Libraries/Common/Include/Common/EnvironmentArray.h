#pragma once

#include <map>
#include <sstream>
#include <stdlib.h>

#if _WIN32
#include <Windows.h>
#else // _WIN32
#error Not implemented
#endif // _WIN32

/** Note: This is not a performant helper, used in one offs */
class EnvironmentArray {
public:
    EnvironmentArray() {
        // Get all strings
        LPCH strings = GetEnvironmentStrings();

        // Parse all keys
        for (size_t i = 0;;) {
            if (strings[i] == '\0') {
                break;
            }

            // Parse name
            size_t begin = i;
            while (strings[i++] != '=');

            // Construct name
            std::string key(strings + begin, i - begin - 1);

            // Parse value
            begin = i;
            while (strings[i++] != '\0');

            // Construct value
            std::string value(strings + begin, i - begin - 1);

            // Add
            keyValueMap[key] << value;
        }
    }

    /// Add a new value
    /// \param key expected key
    /// \param content content to be added
    /// \param delim delimiter used in case previous content exists
    void Add(const std::string& key, const std::string& content, const std::string& delim = ";") {
        auto&& value = keyValueMap[key];

        if (value.tellp() != std::streampos(0)) {
            value << delim;
        }

        value << content;
    }

    /// Compose a standard OS environment block
    /// \return block string
    std::string ComposeEnvironmentBlock() {
        std::stringstream block;

        // Write variables
        for (auto&& kv : keyValueMap) {
            block << kv.first << "=" << kv.second.str() << '\0';
        }

        // Close
        block << '\0';
        return block.str();
    }

private:
    /// All values
    std::map<std::string, std::stringstream> keyValueMap;
};
