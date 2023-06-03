#pragma once

// Std
#include <cstdint>

enum class DXBCPhysicalBlockType : uint32_t {
    /// SM4-5
    Interface = 'EFCI',
    Input = 'NGSI',
    Output5 = '5GSO',
    Output = 'NGSO',
    Patch = 'GSCP',
    Resource = 'FEDR',
    ShaderDebug0 = 'BGDS',
    FeatureInfo = '0IFS',
    Shader4 = 'RDHS',
    Shader5 = 'XEHS',
    ShaderHash = 'HSAH',
    ShaderDebug1 = 'BDPS',
    Statistics = 'TATS',
    PipelineStateValidation = '0VSP',
    RootSignature = '0STR',

    /// SM6
    ILDB = 'BDLI',
    ILDN = 'NDLI',
    DXIL = 'LIXD',
    InputSignature = '1GSI',
    OutputSignature = '1GSO',

    /// Unknown block
    Unexposed = ~0u
};
