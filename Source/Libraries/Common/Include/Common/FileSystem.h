#pragma once

// Std
#include <filesystem>

/// Read all text data to a string
/// \param path path of the file
/// \return text contents
std::string ReadAllText(const std::filesystem::path& path);

/// Get the executable directory
/// \return empty if failed
std::filesystem::path GetCurrentExecutableDirectory();

/// Get the executable name
/// \return empty if failed
std::string GetCurrentExecutableName();

/// Get the module directory
/// \return empty if failed
std::filesystem::path GetCurrentModuleDirectory();

/// Get the base module directory
/// \return empty if failed
std::filesystem::path GetBaseModuleDirectory();

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

/// Sanitize a given path
std::string SanitizePath(const std::string_view &view);
