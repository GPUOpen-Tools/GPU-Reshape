// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

/// Check if a path exists
bool PathExists(const std::string& view);
