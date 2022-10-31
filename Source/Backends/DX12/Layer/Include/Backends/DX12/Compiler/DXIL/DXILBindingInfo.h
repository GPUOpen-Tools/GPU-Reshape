#pragma once

// Layer
#include "DXILHeader.h"
#include <Backends/DX12/States/RootRegisterBindingInfo.h>

// Std
#include <cstdint>

struct DXILBindingInfo {
    /// Target register space
    uint32_t space{};

    /// Handle for shader export data
    uint32_t shaderExportHandleId{};

    /// Handle for PRMT relocation
    uint32_t prmtHandleId{};

    /// Handle for descriptor data
    uint32_t descriptorConstantsHandleId{};

    /// Handle for event data
    uint32_t eventConstantsHandleId{};

    /// Pipeline binding info
    RootRegisterBindingInfo bindingInfo{};

    /// Added features
    DXILProgramShaderFlagSet shaderFlags{};
};
