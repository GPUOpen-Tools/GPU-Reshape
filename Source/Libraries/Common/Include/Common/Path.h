#pragma once

// Std
#include <filesystem>

/// Get the executable directory
/// \return empty if failed
std::filesystem::path CurrentExecutableDirectory();

/// Get the executable name
/// \return empty if failed
std::string CurrentExecutableName();

/// Get the module directory
/// \return empty if failed
std::filesystem::path CurrentModuleDirectory();
