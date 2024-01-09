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
#include <Common/Assert.h>
#include <Common/Enum.h>

// Std
#include <cstdint>

struct DXILHeader {
    uint16_t programVersion;
    uint16_t programType;
    uint32_t dwordCount;
    uint32_t identifier;
    uint32_t version;
    uint32_t codeOffset;
    uint32_t codeSize;
};

enum class DXILAddressSpace {
    Local = 0,
    Device = 1,
    Constant = 2,
    GroupShared = 3
};

enum class DXILShaderResourceClass {
    SRVs = 0,
    UAVs = 1,
    CBVs = 2,
    Samplers = 3,
    Count
};

enum class DXILShaderResourceShape {
    Invalid = 0,
    Texture1D = 1,
    Texture2D = 2,
    Texture2DMS = 3,
    Texture3D = 4,
    TextureCube = 5,
    Texture1DArray = 6,
    Texture2DArray = 7,
    Texture2DMSArray = 8,
    TextureCubeArray = 9,
    TypedBuffer = 10,
    RawBuffer = 11,
    StructuredBuffer = 12,
    CBuffer = 13,
    Sampler = 14,
    TBuffer = 15,
    RTAccelerationStructure = 16,
    FeedbackTexture2D = 17,
    FeedbackTexture2DArray = 18
};

enum class DXILProgramTag {
    ShaderFlags = 0,
    GSState,
    DSState,
    HSState,
    NumThreads,
    AutoBindingsSpace,
    RayPayloadSize,
    RayAttributeSize,
    ShaderKind,
    MSState,
    ASState,
    WaveSize,
    EntryRootSignature
};

enum class DXILShadingModelClass {
    CS,
    VS,
    PS,
    GS,
    HS,
    DS,
    AS,
    MS
};

enum class DXILProgramShaderFlag {
    NoOpt = 1 << 0,
    NoMathRefactor = 1 << 1,
    UseDouble = 1 << 2,
    ForceEarlyDepthStencil = 1 << 3,
    EnableRawAndStructuredBuffers = 1 << 4,
    UseMinPrecision = 1 << 5,
    UseDoubleExtensionIntrinsics = 1 << 6,
    UseMSAD = 1 << 7,
    FullBindings = 1 << 8,
    EnableViewportAndRTArray = 1 << 9,
    UseInnerCoverage = 1 << 10,
    UseStencil = 1 << 11,
    UseTiledResourceIntrinsics = 1 << 12,
    UseRelaxedTypedUAVLoads = 1 << 13,
    UseLevel9ComparisonFiltering = 1 << 14,
    Use64UAVs = 1 << 15,
    UseUAVs = 1 << 16,
    UseCS4RawAndStructuredBuffers = 1 << 17,
    UseROVs = 1 << 18,
    UseWaveIntrinsics = 1 << 19,
    UseIn64Instructions = 1 << 20
};

BIT_SET(DXILProgramShaderFlag);

enum class DXILSRVTag {
    ElementType = 0,
    ByteStride = 1
};

enum class DXILUAVTag {
    ElementType = 0,
    ByteStride = 1
};

enum class DXILCBVTag {

};

enum class ComponentType : uint32_t {
    None = 0,
    Int1 = 1,
    Int16 = 2,
    UInt16 = 3,
    Int32 = 4,
    UInt32 = 5,
    Int64 = 6,
    UInt64 = 7,
    FP16 = 8,
    FP32 = 9,
    FP64 = 10,
    SNormFP16 = 11,
    UNormFP16 = 12,
    SNormFP32 = 13,
    UNormFP32 = 14,
    SNormFP64 = 15,
    UNormFP64 = 16,
    PackedS8x32 = 17,
    PackedU8x32 = 18
};

enum class DXILAtomicBinOp {
    Add = 0,
    And = 1,
    Or = 2,
    XOr = 3,
    IMin = 4,
    IMax = 5,
    UMin = 6,
    UMax = 7,
    Exchange = 8,
    Invalid = 9
};

union DXILResourceBasicProperties {
    struct {
        uint8_t shape : 8; // DXILShaderResourceShape
        uint8_t align : 4;
        uint8_t isUAV : 1;
        uint8_t isROV : 1;
        uint8_t isGloballyCoherent : 1;
        uint8_t samplerCmpOrhasCounter : 1;
        uint8_t reserved2 : 8;
        uint8_t reserved3 : 8;
    };
    
    uint32_t opaque;
};

union DXILResourceTypedProperties {
    struct {
        uint8_t componentType : 8; // ComponentType
        uint8_t componentCount : 8;
        uint8_t sampleCount : 8;
        uint8_t reserved : 8;
    } resource;

    uint32_t structByteStride;
    uint32_t samplerFeedbackType;
    uint32_t cbufferByteSize;
    uint32_t opaque;
};

struct DXILResourceProperties {
    DXILResourceBasicProperties basic;
    DXILResourceTypedProperties typed;
};

inline bool IsBuffer(DXILShaderResourceShape shape) {
    switch (shape) {
        default:
            ASSERT(false, "Unexpected resource shape");
            return false;
        case DXILShaderResourceShape::CBuffer:
        case DXILShaderResourceShape::Texture1D:
        case DXILShaderResourceShape::Texture2D:
        case DXILShaderResourceShape::Texture2DMS:
        case DXILShaderResourceShape::Texture3D:
        case DXILShaderResourceShape::TextureCube:
        case DXILShaderResourceShape::Texture1DArray:
        case DXILShaderResourceShape::Texture2DArray:
        case DXILShaderResourceShape::Texture2DMSArray:
        case DXILShaderResourceShape::TextureCubeArray:
        case DXILShaderResourceShape::FeedbackTexture2D:
        case DXILShaderResourceShape::FeedbackTexture2DArray:
        case DXILShaderResourceShape::RTAccelerationStructure:
            return false;
        case DXILShaderResourceShape::TypedBuffer:
        case DXILShaderResourceShape::RawBuffer:
        case DXILShaderResourceShape::StructuredBuffer:
            return true;
    }
}

inline uint32_t GetShapeComponentCount(DXILShaderResourceShape shape) {
    switch (shape) {
        default:
            ASSERT(false, "Unexpected resource shape");
        return false;
        case DXILShaderResourceShape::CBuffer:
        case DXILShaderResourceShape::TypedBuffer:
        case DXILShaderResourceShape::RawBuffer:
        case DXILShaderResourceShape::StructuredBuffer:
            return 1u;
        case DXILShaderResourceShape::Texture1D:
        case DXILShaderResourceShape::Texture2D:
        case DXILShaderResourceShape::Texture2DMS:
        case DXILShaderResourceShape::Texture1DArray:
        case DXILShaderResourceShape::FeedbackTexture2D:
        case DXILShaderResourceShape::FeedbackTexture2DArray:
            return 2u;
        case DXILShaderResourceShape::Texture3D:
        case DXILShaderResourceShape::TextureCube:
        case DXILShaderResourceShape::Texture2DArray:
        case DXILShaderResourceShape::Texture2DMSArray:
        case DXILShaderResourceShape::TextureCubeArray:
            return 3u;
    }
}
