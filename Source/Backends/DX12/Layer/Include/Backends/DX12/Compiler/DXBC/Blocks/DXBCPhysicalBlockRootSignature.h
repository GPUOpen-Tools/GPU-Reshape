#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

/// Shader block
struct DXBCPhysicalBlockRootSignature : public DXBCPhysicalBlockSection {
    using DXBCPhysicalBlockSection::DXBCPhysicalBlockSection;

    /// Parse validation
    void Parse();

    /// Compile validation
    void Compile();

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockRootSignature& out);

private:
    /// Compile the shader export parameters
    void CompileShaderExport();

private:
    struct DescriptorTable {
        /// All ranges within this table
        std::vector<DXBCRootSignatureDescriptorRange1> ranges;
    };

    struct RootParameter {
        /// Type of this parameter
        DXBCRootSignatureParameterType type;

        /// Shader stage visibility
        DXBCRootSignatureVisibility visibility;

        /// Payload data
        DescriptorTable descriptorTable;
        DXBCRootSignatureParameter1 parameter1;
        DXBCRootSignatureConstant constant;
    };

private:
    /// Root signature header
    DXBCRootSignatureHeader header;

    /// All parameters within this root signature
    std::vector<RootParameter> parameters;

    /// All samplers within this root signature
    std::vector<DXBCRootSignatureSamplerStub> samplers;
};
