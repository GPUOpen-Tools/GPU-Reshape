#pragma once

// Std
#include <cstdint>

/// Shader header
struct DXBCHeader {
    uint32_t identifier;
    uint8_t privateChecksum[16];
    uint32_t reserved;
    uint32_t byteCount;
    uint32_t chunkCount;
};

/// Post header chunk entry header
struct DXBCChunkEntryHeader {
    uint32_t offset;
};

/// Chunk header at offset specified
struct DXBCChunkHeader {
    uint32_t type;
    uint32_t size;
};

/// Shader header
struct DXBCShaderHeader : public DXBCChunkHeader {
    uint8_t minor: 4;
    uint8_t major: 4;
    uint8_t type;
    uint32_t dwordCount;
};

enum class DXBCPSVBindInfoType : uint32_t {
    Invalid = 0,
    Sampler = 1,
    CBuffer = 2,
    ShaderResourceView = 3,
    ShaderResourceViewByte = 4,
    ShaderResourceViewStructured = 5,
    UnorderedAccessView = 6,
    UnorderedAccessViewByte = 7,
    UnorderedAccessViewStructured = 8,
    UnorderedAccessViewCounter = 9
};

enum class DXBCPSVBindInfoKind : uint32_t {
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

enum class DXBCPSVBindInfoFlag {
    None = 0,
    Atomic64 = 1,
};

struct DXBCPSVBindInfo0 {
    DXBCPSVBindInfoType type;
    uint32_t space;
    uint32_t low;
    uint32_t high;
};

struct DXBCPSVBindInfo1 {
    DXBCPSVBindInfoKind kind;
    uint32_t flags;
};

struct DXBCPSVBindInfoRevision0 {
    DXBCPSVBindInfo0 info0;
};

struct DXBCPSVBindInfoRevision1 {
    DXBCPSVBindInfo0 info0;
    DXBCPSVBindInfo1 info1;
};
