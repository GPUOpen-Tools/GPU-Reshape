#pragma once

// Layer
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include "DXBCPhysicalBlockSection.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

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
    /// All bindings
    TrivialStackVector<DXBCPSVBindInfoRevision1, 8u> cbvs;
    TrivialStackVector<DXBCPSVBindInfoRevision1, 8u> srvs;
    TrivialStackVector<DXBCPSVBindInfoRevision1, 8u> uavs;
    TrivialStackVector<DXBCPSVBindInfoRevision1, 8u> samplers;

private:
    /// Size of the runtime info
    uint32_t runtimeInfoSize{};

    /// Size of the runtime bindings
    uint32_t runtimeBindingsSize{};

    /// Dangling offset
    size_t offset{};

    /// Runtime info, revision depends on the size
    DXBCPSVRuntimeInfoRevision2 runtimeInfo{};
};
