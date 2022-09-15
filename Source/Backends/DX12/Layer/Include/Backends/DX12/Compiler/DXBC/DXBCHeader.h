#pragma once

// Common
#include <Common/Enum.h>

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

enum class DXBCShaderFeature {
    UseDouble = 1 << 0,
    ComputeShadersPlusRawAndStructuredBuffersViaShader4X = 1 << 1,
    UAVsAtEveryStage = 1 << 2,
    Use64UAVs = 1 << 3,
    MinimumPrecision = 1 << 4,
    Use11_1_DoubleExtensions = 1 << 5,
    Use11_1_ShaderExtensions = 1 << 6,
    LEVEL9ComparisonFiltering = 1 << 7,
    TiledResources = 1 << 8,
    StencilRef = 1 << 9,
    InnerCoverage = 1 << 10,
    TypedUAVLoadAdditionalFormats = 1 << 11,
    ROVs = 1 << 12,
    ViewportAndRTArrayIndexFromAnyShaderFeedingRasterizer = 1 << 13,
    WaveOps = 1 << 14,
    Int64Ops = 1 << 15,
    ViewID = 1 << 16,
    Barycentrics = 1 << 17,
    NativeLowPrecision = 1 << 18,
    ShadingRate = 1 << 19,
    Raytracing_Tier_1_1 = 1 << 20,
    SamplerFeedback = 1 << 21,
    AtomicInt64OnTypedResource = 1 << 22,
    AtomicInt64OnGroupShared = 1 << 23,
    DerivativesInMeshAndAmpShaders = 1 << 24,
    ResourceDescriptorHeapIndexing = 1 << 25,
    SamplerDescriptorHeapIndexing = 1 << 26,
    AtomicInt64OnHeapResource = 1 << 27,
    AdvancedTextureOps = 1 << 28,
    WriteableMSAATextures = 1 << 29
};

BIT_SET(DXBCShaderFeature);

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

enum class DXBCRootSignatureVersion : uint32_t {
    Version0 = 1,
    Version1 = 2
};

struct DXBCRootSignatureHeader {
    DXBCRootSignatureVersion version;
    uint32_t parameterCount;
    uint32_t rootParameterOffset;
    uint32_t staticSamplerCount;
    uint32_t staticSamplerOffset;
    uint32_t flags;
};

struct DXBCRootSignatureParameter1 {
    uint32_t _register;
    uint32_t space;
    uint32_t flags;
};

struct DXBCRootSignatureConstant {
    uint32_t _register;
    uint32_t space;
    uint32_t dwordCount;
};

enum class DXBCRootSignatureRangeType : uint32_t {
    SRV = 0,
    UAV = 1,
    CBV = 2,
    Sampler = 3
};

enum class DXBCRootSignatureParameterType : uint32_t {
    DescriptorTable = 0,
    Constant32 = 1,
    CBV = 2,
    SRV = 3,
    UAV = 4
};

enum class DXBCRootSignatureVisibility : uint32_t {
    All = 0,
    Vertex = 1,
    Hull = 2,
    Domain = 32,
    Geometry = 4,
    Pixel = 5,
    Amplification = 6,
    Mesh = 7
};

struct DXBCRootSignatureDescriptorRange {
    DXBCRootSignatureRangeType type;
    uint32_t descriptorCount;
    uint32_t _register;
    uint32_t space;
    uint32_t offsetFromTableStart;
};

struct DXBCRootSignatureDescriptorRange1 {
    DXBCRootSignatureRangeType type;
    uint32_t descriptorCount;
    uint32_t _register;
    uint32_t space;
    uint32_t flags;
    uint32_t offsetFromTableStart;
};

struct DXBCRootSignatureDescriptorTable {
    uint32_t rangeCount;
    uint32_t rangeOffset;
};

struct DXILRootSignatureParameter {
    DXBCRootSignatureParameterType type;
    DXBCRootSignatureVisibility visibility;
    uint32_t payloadOffset;
};

struct DXBCRootSignatureSamplerStub {
    uint32_t filter;
    uint32_t addressU;
    uint32_t addressV;
    uint32_t addressW;
    float mipLODBias;
    uint32_t maxAnisotropy;
    uint32_t comparisonFunc;
    uint32_t borderColor;
    float minLOD;
    float maxLOD;
    uint32_t _register;
    uint32_t space;
    uint32_t visibility;
};
