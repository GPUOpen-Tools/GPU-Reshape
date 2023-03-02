#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

/// Shader block
struct DXBCPhysicalBlockRootSignature : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockRootSignature(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

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
        DescriptorTable(const Allocators& allocators) : ranges(allocators) {

        }

        /// All ranges within this table
        Vector<DXBCRootSignatureDescriptorRange1> ranges;
    };

    struct RootParameter {
        RootParameter(const Allocators& allocators) : descriptorTable(allocators) {

        }

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
    Vector<RootParameter> parameters;

    /// All samplers within this root signature
    Vector<DXBCRootSignatureSamplerStub> samplers;
};
