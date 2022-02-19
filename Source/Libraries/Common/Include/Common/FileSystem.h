#pragma once

// Std
#include <filesystem>

/// Get the executable directory
/// \return empty if failed
std::filesystem::path GetCurrentExecutableDirectory();

/// Get the executable name
/// \return empty if failed
std::string GetCurrentExecutableName();

/// Get the module directory
/// \return empty if failed
std::filesystem::path GetCurrentModuleDirectory();

/// Ensure a directory tree exists
/// \param path
void CreateDirectoryTree(const std::filesystem::path& path);

/// Get the shared intermediate path
/// \param category category appended to the base intermediate path
std::filesystem::path GetIntermediatePath(const std::string_view& category);

/// Get the debug path
std::filesystem::path GetIntermediateDebugPath();

/// Get the cache path
std::filesystem::path GetIntermediateCachePath();
