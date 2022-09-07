#pragma once

// Layer
#include <Backends/DX12/States/ShaderInstrumentationKey.h>

// Common
#include <Common/ComRef.h>

// Forward declarations
class DXILSigner;
class DXBCSigner;

/// Job description
struct DXJob {
    /// The instrumentation key
    ShaderInstrumentationKey instrumentationKey{};

    /// The number of streams
    uint32_t streamCount{0};

    /// Signers
    ComRef<DXILSigner> dxilSigner;
    ComRef<DXBCSigner> dxbcSigner;
};
