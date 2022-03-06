#pragma once

// Format
#define FMT_CONSTEVAL
#include <fmt/core.h>

/// Format a message
///   ? See https://github.com/fmtlib/fmt/tree/8.1.1 for formatting documentation
/// \param message message contents
/// \param args arguments to message contents
template<typename... T>
std::string Format(const char* message, T&&... args) {
    return fmt::format(message, args...);
}
