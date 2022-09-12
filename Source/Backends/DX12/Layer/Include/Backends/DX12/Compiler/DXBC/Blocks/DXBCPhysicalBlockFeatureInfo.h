#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

/// Shader block
struct DXBCPhysicalBlockFeatureInfo : public DXBCPhysicalBlockSection {
    using DXBCPhysicalBlockSection::DXBCPhysicalBlockSection;

    /// Parse validation
    void Parse();

    /// Compile validation
    void Compile();

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockFeatureInfo& out);

private:
    DXBCShaderFeatureSet features{};
};
