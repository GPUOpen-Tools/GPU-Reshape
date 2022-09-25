#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "RootRegisterBindingInfo.h"

// Common
#include <Common/Allocators.h>

struct RootSignatureState {
    ~RootSignatureState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Root binding information
    RootRegisterBindingInfo rootBindingInfo;

    /// Number of user parameters
    uint32_t userRootCount;
};
