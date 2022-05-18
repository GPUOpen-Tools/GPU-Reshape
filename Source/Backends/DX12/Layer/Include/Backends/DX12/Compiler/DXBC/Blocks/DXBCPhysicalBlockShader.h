#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"

/// Shader block
struct DXBCPhysicalBlockShader : public DXBCPhysicalBlockSection {
    using DXBCPhysicalBlockSection::DXBCPhysicalBlockSection;

    /// Parse all instructions
    void Parse();
};
