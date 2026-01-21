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

#include <Backends/DX12/Programs/StateObjectPrograms.h>
#include <Backends/DX12/Modules/ShaderRecordPatchingD3D12.h>
#include <Backends/DX12/Modules/RaytracingIndirectSetupD3D12.h>

// Shared
#include <Shared/RaytracingIndirectSetup.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

static bool CreateSBTPatchProgram(const Allocators& allocators, ID3D12Device* device, StateObjectPrograms& program) {
    // See shader for ranges
    D3D12_DESCRIPTOR_RANGE ranges[] = {
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 4,
            .BaseShaderRegister = 1,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
    };

    // Set parameters, keep immutable stuff in the set, keep mutable stuff in root descriptors
    D3D12_ROOT_PARAMETER parameters[] = {
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1u,
                .pDescriptorRanges = ranges
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor = {
                .ShaderRegister = 0,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
            .Descriptor = {
                .ShaderRegister = 5,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 6,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 7,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 8,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        }
    };

    // Single parameter
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 6u;
    desc.pParameters = parameters;

    // Serialize signature
    ID3DBlob* blob;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, nullptr))) {
        ASSERT(false, "Failed to serialize root signature");
        return false;
    }

    // Create root signature
    if (FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&program.sbtPatchRootSignature)))) {
        ASSERT(false, "Failed to create root signature");
        return false;
    }

    // Setup the compute state
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
    computeDesc.CS.pShaderBytecode = reinterpret_cast<const uint32_t*>(kShaderRecordPatchingD3D12);
    computeDesc.CS.BytecodeLength = static_cast<uint32_t>(sizeof(kShaderRecordPatchingD3D12));
    computeDesc.pRootSignature = program.sbtPatchRootSignature;

    // Finally, create the pipeline
    HRESULT result = device->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&program.sbtPatchPipelineState));
    if (FAILED(result)) {
        ASSERT(false, "Failed to create pipeline state");
        return false;
    }

    // OK
    return true;
}

static bool CreateRaytracingIndirectSetupProgram(const Allocators& allocators, ID3D12Device* device, StateObjectPrograms& program) {
    // Set parameters
    D3D12_ROOT_PARAMETER parameters[] = {
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor = {
                .ShaderRegister = 0,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 1,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 2,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 3,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
            .Descriptor = {
                .ShaderRegister = 4,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
            .Descriptor = {
                .ShaderRegister = 5,
                .RegisterSpace = 0
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
    };

    // Single parameter
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 6u;
    desc.pParameters = parameters;

    // Serialize signature
    ID3DBlob* blob;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, nullptr))) {
        ASSERT(false, "Failed to serialize root signature");
        return false;
    }

    // Create root signature
    if (FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&program.raytracingIndirectSetupRootSignature)))) {
        ASSERT(false, "Failed to create root signature");
        return false;
    }

    // Setup the compute state
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
    computeDesc.CS.pShaderBytecode = reinterpret_cast<const uint32_t*>(kRaytracingIndirectSetupD3D12);
    computeDesc.CS.BytecodeLength = static_cast<uint32_t>(sizeof(kRaytracingIndirectSetupD3D12));
    computeDesc.pRootSignature = program.raytracingIndirectSetupRootSignature;

    // Finally, create the pipeline
    HRESULT result = device->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&program.raytracingIndirectSetupPipelineState));
    if (FAILED(result)) {
        ASSERT(false, "Failed to create pipeline state");
        return false;
    }

    // All arguments
    TrivialStackVector<D3D12_INDIRECT_ARGUMENT_DESC, RaytracingIndirectSetupCommandRangeStride> arguments;

    // Constants
    arguments.Add(D3D12_INDIRECT_ARGUMENT_DESC{
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW,
        .ConstantBufferView = {
            .RootParameterIndex = 1
        }
    });

    // Dispatch buffer
    arguments.Add(D3D12_INDIRECT_ARGUMENT_DESC{
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW,
        .ShaderResourceView = {
            .RootParameterIndex = 2
        }
    });

    // Diagnostic
    arguments.Add(D3D12_INDIRECT_ARGUMENT_DESC{
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW,
        .UnorderedAccessView = {
            .RootParameterIndex = 3
        }
    });

    // Patch Signature
    arguments.Add(D3D12_INDIRECT_ARGUMENT_DESC{
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW,
        .UnorderedAccessView = {
            .RootParameterIndex = 4
        }
    });

    // Actual SBT patch dispatch
    arguments.Add(D3D12_INDIRECT_ARGUMENT_DESC{
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH
    });

    // Validation
    ASSERT(arguments.Size() == RaytracingIndirectSetupCommandRangeStride, "Unexpected count");

    // Setup signatur edesc
    D3D12_COMMAND_SIGNATURE_DESC signatureDesc{};
    signatureDesc.ByteStride = RaytracingIndirectSetupCommandByteStride;
    signatureDesc.pArgumentDescs = arguments.Data();
    signatureDesc.NumArgumentDescs = static_cast<UINT>(arguments.Size());

    // Try to create command signature
    result = device->CreateCommandSignature(&signatureDesc, program.sbtPatchRootSignature, __uuidof(ID3D12CommandSignature), reinterpret_cast<void**>(&program.raytracingIndirectSetupCommandSignature));
    if (FAILED(result)) {
        ASSERT(false, "Failed to create pipeline state");
        return false;
    }
    
    // OK
    return true;
}

bool CreateStateObjectPrograms(const Allocators& allocators, ID3D12Device* device, StateObjectPrograms& program) {
    if (!CreateSBTPatchProgram(allocators, device, program)) {
        return false;
    }

    if (!CreateRaytracingIndirectSetupProgram(allocators, device, program)) {
        return false;
    }

    // OK
    return true;
}

StateObjectPrograms::~StateObjectPrograms() {
    if (sbtPatchRootSignature) {
        sbtPatchRootSignature->Release();
    }
    
    if (sbtPatchPipelineState) {
        sbtPatchPipelineState->Release();
    }
    
    if (raytracingIndirectSetupRootSignature) {
        raytracingIndirectSetupRootSignature->Release();
    }
    
    if (raytracingIndirectSetupPipelineState) {
        raytracingIndirectSetupPipelineState->Release();
    }
}
