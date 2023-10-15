// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
        currentStr = templateStr;

        // OK
        return true;
    }

    /// Substitute a templated entry, only a single instance
    /// \param key the entry to look for
    /// \param value the value to be replaced with
    /// \return success
    bool Substitute(const char* key, const char* value) {
        // Try to find the entry
        auto index = currentStr.find(key);
        if (index == std::string::npos) {
            return false;
        }

        // Replace
        currentStr.replace(index, std::strlen(key), value);

        // OK
        return true;
    }

    /// Substitute all templated entries
    /// \param key the entry to look for
    /// \param value the value to be replaced with
    /// \return success
    void SubstituteAll(const char* key, const char* value) {
        while (Substitute(key, value));
    }

    /// Reset this template
    void Reset() {
        currentStr = templateStr;
    }

    /// Get the instantiated template
    const std::string& GetString() const {
        return currentStr;
    }

private:
    std::string currentStr;
    std::string templateStr;
};
