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

struct DXBCPSVVSInfo {
    char hasOutputPosition;
};

struct DXBCPSVHSInfo {
    uint32_t inputControlCount;
    uint32_t outputControlCount;
    uint32_t tessellatorDomain;
    uint32_t tessellatorOutputPrimitive;
};

struct DXBCPSVDSInfo {
    uint32_t inputControlCount;
    char hasOutputPosition;
    uint32_t tessellatorDomain;
};

struct DXBCPSVGSInfo {
    uint32_t inputPrimitive;
    uint32_t outputTopology;
    uint32_t outputStreamMask;
    char hasOutputPosition;
};

struct DXBCPSVPSInfo {
    char hasDepthOutput;
    char sampleFrequency;
};

struct DXBCPSVMSInfo {
    uint32_t groupSharedByteCount;
    uint32_t groupSharedDependentByteCount;
    uint32_t payloadByteCount;
    uint16_t maxVertexCount;
    uint16_t maxPrimitiveCount;
};

struct DXBCPSVASInfo {
    uint32_t payloadByteCount;
};

struct DXBCPSVMSInfo1 {
    uint8_t primVectors;
    uint8_t topology;
};

struct DXBCPSVRuntimeInfo0 {
    union {
        DXBCPSVVSInfo vs;
        DXBCPSVHSInfo hs;
        DXBCPSVDSInfo ds;
        DXBCPSVGSInfo gs;
        DXBCPSVPSInfo ps;
        DXBCPSVMSInfo ms;
        DXBCPSVASInfo as;
    };

    uint32_t minWaveCount;
    uint32_t maxWaveCount;
};

struct DXBCPSVRuntimeInfo1 {
    uint8_t stage;
    uint8_t hasViewID;

    union {
        uint16_t maxVertexCount;
        uint8_t patchConstOrPrimVectors;
        DXBCPSVMSInfo1 ms1;
    };

    uint8_t inputElements;
    uint8_t outputElements;
    uint8_t patchConstOrPrimElements;
    uint8_t inputVectors;
    uint8_t outputVectors[4];
};

struct DXBCPSVRuntimeInfo2 {
    uint32_t threadCountX;
    uint32_t threadCountY;
    uint32_t threadCountZ;
};

struct DXBCPSVRuntimeInfoRevision0 {
    DXBCPSVRuntimeInfo0 info0;
};

struct DXBCPSVRuntimeInfoRevision1 {
    DXBCPSVRuntimeInfo0 info0;
    DXBCPSVRuntimeInfo1 info1;
};

struct DXBCPSVRuntimeInfoRevision2 {
    DXBCPSVRuntimeInfo0 info0;
    DXBCPSVRuntimeInfo1 info1;
    DXBCPSVRuntimeInfo2 info2;
};

enum class DXBCRootSignatureVersion : uint32_t {
    Version0 = 1,
    Version1 = 2
};

enum class DXBCRootSignatureFlags : uint32_t {
    None = 0,
    AllowInputAssemblerInputLayout = 1,
    DenyVertexShaderRootAccess = 2,
    DenyHullShaderRootAccess = 4,
    DenyDomainShaderRootAccess = 8,
    DenyGeometryShaderRootAccess = 16,
    DenyPixelShaderRootAccess = 32,
    AllowStreamOutput = 64,
    LocalRootSignature = 128,
    DenyAmplificationShaderRootAccess = 256,
    DenyMeshShaderRootAccess = 512,
    CBVSRVUAVHeapDirectlyIndexed = 1024,
    SamplerHeapDirectlyIndexed = 2048,
    AllowLowTierReservedHwCbLimit = 2147483648
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

enum class DXILSignatureElementSemantic : uint32_t {
    Undefined = 0,
    Position = 1,
    ClipDistance = 2,
    CullDistance = 3,
    RenderTargetArrayIndex = 4,
    ViewPortArrayIndex = 5,
    VertexID = 6,
    PrimitiveID = 7,
    InstanceID = 8,
    IsFrontFace = 9,
    SampleIndex = 10,
    FinalQuadEdgeTessFactor = 11,
    FinalQuadInsideTessFactor = 12,
    FinalTriEdgeTessFactor = 13,
    FinalTriInsideTessFactor = 14,
    FinalLineDetailTessFactor = 15,
    FinalLineDensityTessFactor = 16,
    Barycentrics = 23,
    ShadingRate = 24,
    CullPrimitive = 25,
    Target = 64,
    Depth = 65,
    Coverage = 66,
    DepthGreaterEqual = 67,
    DepthLessEqual = 68,
    StencilRef = 69,
    InnerCoverage = 70,
};

enum class DXILSignatureElementComponentType : uint32_t {
    Unknown = 0,
    UInt32 = 1,
    Int32 = 2,
    Float32 = 3,
    UInt16 = 4,
    Int16 = 5,
    Float16 = 6,
    UInt64 = 7,
    Int64 = 8,
    Float64 = 9,
};

enum class DXILSignatureElementPrecision : uint32_t {
    Default = 0,
    Float16 = 1,
    Float2_8 = 2,
    Reserved = 3,
    Int16 = 4,
    UInt16 = 5,
    Any16 = 0xf0,
    Any10 = 0xf1
};

struct DXILInputSignature {
    uint32_t count;
    uint32_t offset;
};

struct DXILSignatureElement {
    uint32_t streamIndex;
    uint32_t semanticNameOffset;
    uint32_t semanticIndex;
    DXILSignatureElementSemantic semantic;
    DXILSignatureElementComponentType componentType;
    uint32_t _register;
    uint8_t mask;
    uint8_t writeMask;
    uint16_t pad;
    DXILSignatureElementPrecision precision;
};

struct DXILShaderDebugName {
    uint16_t flags;
    uint16_t nameLength;
};

struct DXILDigest {
    uint8_t digest[16];
};

struct DXILShaderHash {
    uint32_t flags;
    DXILDigest digest;
};

enum class DXILPDBVersion : uint32_t {

};

struct DXILPDBHeader {
    DXILPDBVersion version;
    uint32_t signature;
    uint32_t age;
    DXILDigest digest;
    uint32_t ignore[7];
};
