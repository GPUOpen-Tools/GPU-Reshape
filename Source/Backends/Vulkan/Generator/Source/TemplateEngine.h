#pragma once

#include <fstream>
#include <sstream>
#include <string>

/// Simple template engine
struct TemplateEngine {
    /// Load a template
    /// \param path the template file path
    /// \return success
    [[nodiscard]]
    bool Load(const char* path) {
        std::ifstream templateFile(path);

        // Try to open the template
        if (!templateFile.good()) {
            return false;
        }

        // Store
        std::stringstream templateSS;
        templateSS << templateFile.rdbuf();
        templateStr = templateSS.str();

        // OK
        return true;
    }

    /// Substitute a templated entry, only a single instance
    /// \param key the entry to look for
    /// \param value the value to be replaced with
    /// \return success
    bool Substitute(const char* key, const char* value) {
        // Try to find the entry
        auto index = templateStr.find(key);
        if (index == std::string::npos) {
            return false;
        }

        // Replace
        templateStr.replace(index, std::strlen(key), value);

        // OK
        return true;
    }

    /// Get the instantiated template
    const std::string& GetString() const {
        return templateStr;
    }

private:
    std::string templateStr;
};
