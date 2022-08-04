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
    Capabilities = '0IFS',
    Shader4 = 'RDHS',
    Shader5 = 'XEHS',
    ShaderDebug1 = 'BDPS',
    Statistics = 'TATS',
    PipelineStateValidation = '0VSP',

    /// SM6
    ILDB = 'BDLI',
    DXIL = 'LIXD',

    /// Unknown block
    Unexposed = ~0u
};
