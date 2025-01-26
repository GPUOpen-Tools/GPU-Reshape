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

// Layer
#include <Backends/DX12/Compiler/DXBC/DXBCRDATRootSignature.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/DX12.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// General traits
template<typename T>
struct RootSignatureTraits;

// Version 1_0 traits
template<>
struct RootSignatureTraits<D3D12_ROOT_SIGNATURE_DESC> {
    using Parameter          = D3D12_ROOT_PARAMETER;
    using DescriptorTable    = D3D12_ROOT_DESCRIPTOR_TABLE;
    using Range              = D3D12_DESCRIPTOR_RANGE;
    using Sampler            = D3D12_STATIC_SAMPLER_DESC;
    using DXBCRootDescriptor = DXBCRDATRootSignatureRootDescriptor;
    using DXBCRange          = DXBCRDATRootSignatureDescriptorRange;
};

// Version 1_1 traits
template<>
struct RootSignatureTraits<D3D12_ROOT_SIGNATURE_DESC1> {
    using Parameter          = D3D12_ROOT_PARAMETER1;
    using DescriptorTable    = D3D12_ROOT_DESCRIPTOR_TABLE1;
    using Range              = D3D12_DESCRIPTOR_RANGE1;
    using Sampler            = D3D12_STATIC_SAMPLER_DESC;
    using DXBCRootDescriptor = DXBCRDATRootSignatureRootDescriptor1;
    using DXBCRange          = DXBCRDATRootSignatureDescriptorRange1;
};

/// All valid descriptor flags, remove RDAT specific data
static constexpr uint64_t kValidDescriptorFlags =
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE |
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE |
    D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

/// All valid descriptor range flags, remove RDAT specific data
static constexpr uint64_t kValidDescriptorRangeFlags =
    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE |
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE |
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE |
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC |
    D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS;

/// All valid root signature flags, remove RDAT specific data
static constexpr uint64_t kValidRootSignatureFlags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT |
    D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
    D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

template<typename T, typename Traits = RootSignatureTraits<T>>
void CreateDXBCRDATRootSignatureVersioned(DXBCParseContext& ctx, const DXBCRDATRootSignatureHeader& header, ID3DBlob** out) {
    LinearBlockAllocator<1024> allocator;

    // Start parsing from parameters
    ctx.SetOffset(header.parameterOffset);

    // Parse all parameters
    auto parameters = allocator.AllocateArray<typename Traits::Parameter>(header.parameterCount);
    for (uint32_t i = 0; i < header.parameterCount; i++) {
        auto srcParameter = ctx.Consume<DXBCRDATRootSignatureRootParameter>();

        // Parse parameter
        typename Traits::Parameter& dstParameter = parameters[i];
        dstParameter.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(srcParameter.type);
        dstParameter.ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(srcParameter.shaderVisibility);

        switch (dstParameter.ParameterType) {
            default: {
                ASSERT(false, "Invalid type");
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                // Parse table
                auto payload = ctx.ReadAt<DXBCRDATRootSignatureRootDescriptorTable>(srcParameter.payloadOffset);

                // Setup context at range
                DXBCParseContext rangeCtx(ctx.start + payload->rangeOffset, sizeof(typename Traits::DXBCRange) * payload->rangeCount);

                // Parse all ranges
                auto ranges = allocator.AllocateArray<typename Traits::Range>(payload->rangeCount);
                for (uint32_t rangeIndex = 0; rangeIndex < payload->rangeCount; rangeIndex++) {
                    auto srcRange = rangeCtx.Consume<typename Traits::DXBCRange>();

                    // Parse range
                    typename Traits::Range& dstRange = ranges[rangeIndex];
                    dstRange.RangeType = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(srcRange.rangeType);
                    dstRange.NumDescriptors = srcRange.descriptorCount;
                    dstRange.BaseShaderRegister = srcRange.baseShaderRegister;
                    dstRange.RegisterSpace = srcRange.registerSpace;
                    dstRange.OffsetInDescriptorsFromTableStart = srcRange.offsetInTable;

                    // V1?
                    if constexpr(std::is_same_v<typename Traits::Range, D3D12_DESCRIPTOR_RANGE1>) {
                        // Remove serialization specific flags
                        dstRange.Flags = static_cast<D3D12_DESCRIPTOR_RANGE_FLAGS>(srcRange.flags & kValidDescriptorRangeFlags);
                    }
                }

                // Set immutable ranges
                dstParameter.DescriptorTable.NumDescriptorRanges = payload->rangeCount;
                dstParameter.DescriptorTable.pDescriptorRanges = ranges;
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                // Parse constants
                auto payload = ctx.ReadAt<DXBCRDATRootSignatureRootConstant>(srcParameter.payloadOffset);
                dstParameter.Constants.ShaderRegister = payload->shaderRegister;
                dstParameter.Constants.RegisterSpace = payload->registerSpace;
                dstParameter.Constants.Num32BitValues = payload->dwordCount;
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_SRV:
            case D3D12_ROOT_PARAMETER_TYPE_UAV:
            case D3D12_ROOT_PARAMETER_TYPE_CBV: {
                // Parse descriptor
                auto payload = ctx.ReadAt<typename Traits::DXBCRootDescriptor>(srcParameter.payloadOffset);
                dstParameter.Descriptor.ShaderRegister = payload->shaderRegister;
                dstParameter.Descriptor.RegisterSpace = payload->registerSpace;

                // V1?
                if constexpr(std::is_same_v<typename Traits::Parameter, D3D12_ROOT_PARAMETER1>) {
                    dstParameter.Descriptor.Flags = static_cast<D3D12_ROOT_DESCRIPTOR_FLAGS>(payload->flags & kValidDescriptorFlags);
                }
                break;
            }
        }
    }

    // Start parsing from samplers
    ctx.SetOffset(header.staticSamplerOffset);

    // Parse samplers
    auto samplers = allocator.AllocateArray<typename Traits::Sampler>(header.staticSamplerCount);
    for (uint32_t i = 0; i < header.staticSamplerCount; i++) {
        auto srcSampler = ctx.Consume<DXBCRDATRootSignatureStaticSampler>();

        // Parse sampler state
        typename Traits::Sampler& dstSampler = samplers[i];
        dstSampler.Filter = srcSampler.filter;
        dstSampler.AddressU = srcSampler.addressU;
        dstSampler.AddressV = srcSampler.addressV;
        dstSampler.AddressW = srcSampler.addressW;
        dstSampler.MipLODBias = srcSampler.mipLODBias;
        dstSampler.MaxAnisotropy = srcSampler.maxAnisotropy;
        dstSampler.ComparisonFunc = srcSampler.comparisonFunc;
        dstSampler.BorderColor = srcSampler.borderColor;
        dstSampler.MinLOD = srcSampler.minLOD;
        dstSampler.MaxLOD = srcSampler.maxLOD;
        dstSampler.ShaderRegister = srcSampler.shaderRegister;
        dstSampler.RegisterSpace = srcSampler.registerSpace;
        dstSampler.ShaderVisibility = srcSampler.shaderVisibility;

        // V1?
        if constexpr(std::is_same_v<typename Traits::Sampler, D3D12_STATIC_SAMPLER_DESC1>) {
            dstSampler.Flags = static_cast<D3D12_SAMPLER_FLAGS>(0);
        }
    }

    // Setup description
    T desc;
    desc.NumParameters = header.parameterCount;
    desc.pParameters = parameters;
    desc.NumStaticSamplers = header.staticSamplerCount;
    desc.pStaticSamplers = samplers;
    desc.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(header.flags & kValidRootSignatureFlags);

    // Versioned creation info
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned;
    if constexpr(std::is_same_v<T, D3D12_ROOT_SIGNATURE_DESC1>) {
        versioned.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        versioned.Desc_1_1 = desc;
    } else {
        versioned.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        versioned.Desc_1_0 = desc;
    }

    // Create it
    D3D12SerializeVersionedRootSignature(
        &versioned,
        out,
        nullptr
    );
}

void CreateDXBCRDATRootSignature(const void* data, uint64_t length, ID3DBlob** out) {
    DXBCParseContext ctx(data, length);

    // Safety
    *out = nullptr;

    // Get header for versioning
    auto header = ctx.Consume<DXBCRDATRootSignatureHeader>();
    switch (header.version) {
        default:
            ASSERT(false, "Invalid version");
            return;
        case DXBCRDATRootSignatureVersion::Version1_0:
            CreateDXBCRDATRootSignatureVersioned<D3D12_ROOT_SIGNATURE_DESC>(ctx, header, out);
            break;
        case DXBCRDATRootSignatureVersion::Version1_1:
            CreateDXBCRDATRootSignatureVersioned<D3D12_ROOT_SIGNATURE_DESC1>(ctx, header, out);
            break;
    }
}
