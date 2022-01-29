#pragma once

// Std
#include <filesystem>

/// Get the executable directory
/// \return empty if failed
std::filesystem::path CurrentExecutableDirectory();

/// Get the module directory
/// \return empty if failed
std::filesystem::path CurrentModuleDirectory();
