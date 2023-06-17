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

// Common
#include <Common/IComponent.h>

// Std
#include <filesystem>
#include <mutex>

// Forward declarations
class DXModule;
struct DeviceState;

class ShaderCompilerDebug : public TComponent<ShaderCompilerDebug> {
public:
    COMPONENT(ShaderCompilerDebug);

    ShaderCompilerDebug();

    /// Allocate a path
    /// \param module module to be allocated for
    /// \return
    std::filesystem::path AllocatePath(DXModule *module);

    /// Allocate a path
    /// \param name decorative name
    /// \return
    std::filesystem::path AllocatePath(const std::string_view& name);

    /// Add a module
    /// \param basePath allocated base path
    /// \param category category of the shader module
    /// \param module module to be serialized
    void Add(const std::filesystem::path& basePath, const std::string_view& category, DXModule *module);

private:
    /// Shared lock for printing
    std::mutex sharedLock;

    /// Base path for all debugging data
    std::filesystem::path path;
};
