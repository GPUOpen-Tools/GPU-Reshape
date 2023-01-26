#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/FeatureProxies.Gen.h>

// Backend
#include <Backend/CommandContext.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DeviceState;
struct ShaderExportStreamState;
class DescriptorDataAppendAllocator;

struct __declspec(uuid("8270D898-4356-4503-8DEB-9CD73BB31B21")) CommandListState {
    ~CommandListState();

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Current streaming state
    ShaderExportStreamState* streamState{nullptr};

    /// Current descriptor state
    DescriptorDataAppendAllocator* descriptorAllocator{nullptr};

    /// The actual list type
    D3D12_COMMAND_LIST_TYPE userType = D3D12_COMMAND_LIST_TYPE_DIRECT;

    /// All contained proxies
    ID3D12GraphicsCommandListFeatureProxies proxies;

    /// User context
    CommandContext userContext;

    /// Object
    ID3D12GraphicsCommandList* object{nullptr};
};
