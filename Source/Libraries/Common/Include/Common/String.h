// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

// Std
#include <algorithm>
#include <cctype>
#include <locale>

namespace std {
    static inline void ltrim(std::string &s) {
        s.erase(0, s.find_first_not_of(" \n\t\v\f\r"));
    }

    static inline void rtrim(std::string &s) {
        s.erase(s.find_last_not_of(" \n\t\v\f\r") + 1u);
    }

    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    static inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    static inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    static inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }

    static inline std::string uppercase(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    static inline bool iequals(const string &a, const string &b) {
        return std::equal(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            }
        );
    }

    static inline string::const_iterator isearch(const string &a, const string_view &b) {
        return std::search(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            }
        );
    }

    static inline string_view::const_iterator isearch(const string_view &a, const string_view &b) {
        return std::search(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            }
        );
    }

    static inline bool icontains(const string_view &a, const string_view &b) {
        return isearch(a, b) != a.end();
    }

    inline bool ends_with(std::string_view const &value, std::string_view const &ending) {
        if (ending.length() > value.length()) {
            return false;
        }
        
        return !value.compare(value.length() - ending.length(), ending.length(), ending);
    }

    inline bool starts_with(std::string_view const &value, std::string_view const &ending) {
        if (ending.length() > value.length()) {
            return false;
        }
        
        return !value.compare(0, ending.length(), ending);
    }

    static inline bool iswhitespace(char c) {
        return isspace(c) && c != '\n';
    }

    static inline bool iscxxalnum(char c) {
        return isalnum(c) || c == '_';
    }
}