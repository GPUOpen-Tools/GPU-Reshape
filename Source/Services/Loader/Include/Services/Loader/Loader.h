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
#include <cstdint>

struct GRSLoaderSymbolInfo {
    /// All paths
    const char** paths = nullptr;

    /// Number of symbol paths
    uint32_t pathCount = 0;

    /// Should subdirectories be scanned?
    bool includeSubDirectories = false;
};

struct GRSLoaderInstallInfo {
    /// All symbol information
    GRSLoaderSymbolInfo symbol;
};

/// Symbol helper
#define GRS_SYMBOL(NAME, STR) \
    [[maybe_unused]] static constexpr const char* k##NAME = STR; \
    [[maybe_unused]] static constexpr const wchar_t* k##NAME##W = L##STR;

/// Loader proc-fn types
using PFN_GRS_LOADER_INSTALL            = bool(*)(const GRSLoaderInstallInfo* info);
using PFN_GRS_LOADER_GET_RESERVED_TOKEN = void(*)(char* output, uint32_t* length);

/// Loader symbols
GRS_SYMBOL(PFNGRSLoaderInstall,          "GRSLoaderInstall");
GRS_SYMBOL(PFNGRSLoaderGetReservedToken, "GRSLoaderGetReservedToken");

/// Install the GPU Reshape loader
/// Ensures that all relevant backends are injected
extern "C" __declspec(dllexport) bool GRSLoaderInstall(const GRSLoaderInstallInfo* info);

/// Get the reserved token for this loader
/// Used for later attaching (e.g., GPUReshape attach -token [...])
extern "C" __declspec(dllexport) void GRSLoaderGetReservedToken(char* output, uint32_t* length);

// Cleanup
#undef GRS_SYMBOL
