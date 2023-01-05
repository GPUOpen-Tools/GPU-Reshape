#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "RootRegisterBindingInfo.h"

// Common
#include <Common/Allocators.h>

// Forward declarations
struct RootSignaturePhysicalMapping;

struct RootSignatureState {
    ~RootSignatureState();

    /// Parent state
    ID3D12Device* parent{};

    /// Device object
    ID3D12RootSignature* object{};

    /// Owning allocator
    Allocators allocators;

    /// Root binding information
    RootRegisterBindingInfo rootBindingInfo;

    /// Number of user parameters
    uint32_t userRootCount;

    /// Contained physical mappings
    RootSignaturePhysicalMapping* physicalMapping{nullptr};
};
