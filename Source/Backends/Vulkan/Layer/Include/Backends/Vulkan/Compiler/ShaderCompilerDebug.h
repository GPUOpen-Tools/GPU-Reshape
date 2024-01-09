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

// Common
#include <Common/IComponent.h>

// Std
#include <filesystem>
#include <mutex>

class SpvModule;
struct InstanceDispatchTable;
struct VkShaderModuleCreateInfo;

namespace spvtools {
    class SpirvTools;
}

class ShaderCompilerDebug : public TComponent<ShaderCompilerDebug> {
public:
    COMPONENT(ShaderCompilerDebug);

    ShaderCompilerDebug(InstanceDispatchTable* table);

    ~ShaderCompilerDebug();

    /// Install this component
    /// \return
    bool Install();

    /// Validate a module
    /// \param module
    /// \return
    bool Validate(const uint32_t* spirvCode, uint64_t spirvSize);

    /// Allocate a path
    /// \param module module to be allocated for
    /// \return
    std::filesystem::path AllocatePath(SpvModule *module);

    /// Allocate a path
    /// \param view decorative name
    /// \return
    std::filesystem::path AllocatePath(const std::string_view& view);

    /// Add a module
    /// \param basePath allocated base path
    /// \param category category of the shader module
    /// \param module module to be serialized
    /// \param spirvCode produced spirv code
    /// \param spirvSize produced spirv size
    void Add(const std::filesystem::path& basePath, const std::string_view& category, SpvModule *module, const void* spirvCode, uint64_t spirvSize);

private:
    InstanceDispatchTable* table;

    /// Shared lock for printing
    std::mutex sharedLock;

    /// Tools handle
    spvtools::SpirvTools* tools{nullptr};

    /// Base path for all debugging data
    std::filesystem::path path;
};
