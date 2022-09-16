#pragma once

// Layer
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
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
    /// Size of the runtime info
    uint32_t runtimeInfoSize{};

    /// Runtime info, revision depends on the size
    DXBCPSVRuntimeInfoRevision2 runtimeInfo{};
};
