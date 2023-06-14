#pragma once

// Layer
#include "RootParameterVisibility.h"
#include "RootSignatureVisibilityClass.h"

struct RootSignaturePhysicalMapping {
    /// Signature hash
    uint64_t signatureHash{0};

    /// All register binding classes
    RootSignatureVisibilityClass visibility[static_cast<uint32_t>(RootParameterVisibility::Count)];
};
