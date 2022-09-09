#pragma once

// Common
#include <Common/Assert.h>

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

enum class DXILUAVTag {
    ElementType = 0,
    ByteStride = 1
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
            return false;
        case DXILShaderResourceShape::TypedBuffer:
        case DXILShaderResourceShape::RawBuffer:
        case DXILShaderResourceShape::StructuredBuffer:
            return true;
    }
}
