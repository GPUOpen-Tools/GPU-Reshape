#pragma once

// Layer
#include <Backends/DX12/States/ShaderInstrumentationKey.h>

// Common
#include <Common/ComRef.h>

// Forward declarations
class DXILSigner;

/// Job description
struct DXJob {
    /// The instrumentation key
    ShaderInstrumentationKey instrumentationKey{};

    /// The number of streams
    uint32_t streamCount{0};

    /// DXIL signer
    ComRef<DXILSigner> dxilSigner;
};
