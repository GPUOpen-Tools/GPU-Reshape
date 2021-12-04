#pragma once

// Std
#include <algorithm>
#include <cctype>
#include <locale>

namespace std {
    // https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
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

    static inline bool iequals(const string &a, const string &b) {
        return std::equal(
            a.begin(), a.end(),
            b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            }
        );
    }
}