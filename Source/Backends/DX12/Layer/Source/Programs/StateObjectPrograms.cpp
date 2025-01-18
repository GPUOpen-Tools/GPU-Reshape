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

bool CreateStateObjectPrograms(const Allocators& allocators, ID3D12Device* device, StateObjectPrograms& program) {
    // See shader for ranges
    D3D12_DESCRIPTOR_RANGE ranges[] = {
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 1,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .NumDescriptors = 2,
            .BaseShaderRegister = 2,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 3,
            .BaseShaderRegister = 4,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
    };

    // All grouped up in a single table
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameter.DescriptorTable.NumDescriptorRanges = 4u;
    parameter.DescriptorTable.pDescriptorRanges = ranges;

    // Single parameter
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = 1;
    desc.pParameters = &parameter;

    // Serialize signature
    ID3DBlob* blob;
    if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, nullptr))) {
        ASSERT(false, "Failed to serialize root signature");
        return false;
    }

    // Create root signature
    if (FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&program.rootSignature)))) {
        ASSERT(false, "Failed to create root signature");
        return false;
    }

    // Setup the compute state
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
    computeDesc.CS.pShaderBytecode = reinterpret_cast<const uint32_t*>(kShaderRecordPatchingD3D12);
    computeDesc.CS.BytecodeLength = static_cast<uint32_t>(sizeof(kShaderRecordPatchingD3D12));
    computeDesc.pRootSignature = program.rootSignature;

    // Finally, create the pipeline
    HRESULT result = device->CreateComputePipelineState(&computeDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&program.pipelineState));
    if (FAILED(result)) {
        ASSERT(false, "Failed to create pipeline state");
        return false;
    }

    // OK
    return true;
}

StateObjectPrograms::~StateObjectPrograms() {
    if (rootSignature) {
        rootSignature->Release();
    }
    
    if (pipelineState) {
        pipelineState->Release();
    }
}
