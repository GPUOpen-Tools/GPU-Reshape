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

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/States/RootRegisterBindingInfo.h>
#include <Backends/DX12/States/RootSignaturePhysicalMapping.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Std
#include <vector>

// Forward declarations
class DXModule;
class ShaderDataHost;
class ShaderCompilerDebug;
struct DeviceState;

class ShaderProgramHost final : public IShaderProgramHost {
public:
    ShaderProgramHost(DeviceState* device);

    /// Install this host
    /// \return success state
    bool Install();

    /// Install this host
    /// \return success state
    bool InstallPrograms();

    /// IShaderProgramHost
    ShaderProgramID Register(const ComRef<IShaderProgram> &program) override;
    void Deregister(ShaderProgramID program) override;

    /// Get the shared signature
    ID3D12RootSignature* GetSignature() const {
        return rootSignature;
    }

    /// Get the pipeline of a program
    ID3D12PipelineState* GetPipeline(ShaderProgramID id) const {
        return programs[id].pipeline;
    }

private:
    /// Create the shared root signature
    /// \return success state
    bool CreateRootSignature();

private:
    struct ProgramEntry {
        /// Templated module
        DXModule* module{nullptr};

        /// Program pipeline
        ID3D12PipelineState* pipeline{nullptr};

        /// Source program
        ComRef<IShaderProgram> program;
    };

    /// All programs, may contain empty slots
    Vector<ProgramEntry> programs;

    /// All free indices
    Vector<ShaderProgramID> freeIndices;

private:
    /// All exposed shader data
    Vector<ShaderDataInfo> shaderData;

private:
    /// Shared root signature
    ID3D12RootSignature* rootSignature{nullptr};

    /// Shared root bindings
    RootRegisterBindingInfo rootBindingInfo;

    /// Shared root physical mappings
    RootSignaturePhysicalMapping* rootPhysicalMapping{nullptr};

private:
    /// Base module used for templating
    DXModule* templateModule{nullptr};

    /// Optional debug handle
    ComRef<ShaderCompilerDebug> debug;

private:
    DeviceState* device{nullptr};
};
