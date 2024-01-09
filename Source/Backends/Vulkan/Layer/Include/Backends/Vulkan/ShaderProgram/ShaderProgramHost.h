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

// Layer
#include <Backends/Vulkan/Layer.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Std
#include <vector>

// Forward declarations
class SpvModule;
class ShaderDataHost;
class ShaderCompilerDebug;
struct DeviceDispatchTable;

class ShaderProgramHost final : public IShaderProgramHost {
public:
    ShaderProgramHost(DeviceDispatchTable* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Install this host
    /// \return success state
    bool InstallPrograms();

    /// IShaderProgramHost
    ShaderProgramID Register(const ComRef<IShaderProgram> &program) override;
    void Deregister(ShaderProgramID program) override;

    /// Get the pipeline layout of a program
    VkPipelineLayout GetPipelineLayout(ShaderProgramID id) {
        return programs[id].layout;
    }

    /// Get the pipeline of a program
    VkPipeline GetPipeline(ShaderProgramID id) {
        return programs[id].pipeline;
    }

private:
    struct ProgramEntry {
        /// Templated module
        SpvModule* module{nullptr};

        /// Program shader module
        VkShaderModule shaderModule{VK_NULL_HANDLE};

        /// Program layout
        VkPipelineLayout layout{VK_NULL_HANDLE};

        /// Program pipeline
        VkPipeline pipeline{VK_NULL_HANDLE};

        /// Source program
        ComRef<IShaderProgram> program;
    };

    /// All programs, may contain empty slots
    std::vector<ProgramEntry> programs;

    /// Free program indices
    std::vector<ShaderProgramID> freeIndices;

private:
    /// All exposed shader data
    std::vector<ShaderDataInfo> shaderData;

private:
    /// Base module being templated against
    SpvModule* templateModule{nullptr};

    /// Optional debug handle
    ComRef<ShaderCompilerDebug> debug;

private:
    DeviceDispatchTable* table{nullptr};
};
