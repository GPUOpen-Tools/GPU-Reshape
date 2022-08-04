#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"

/// Shader block
struct DXBCPhysicalBlockPipelineStateValidation : public DXBCPhysicalBlockSection {
    using DXBCPhysicalBlockSection::DXBCPhysicalBlockSection;

    /// Parse validation
    void Parse();

    /// Compile validation
    void Compile();

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockPipelineStateValidation& out);

private:
    size_t resourceOffset{};
};
