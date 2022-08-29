#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

#include <vector>

// Forward declarations
struct DeviceState;
struct ShaderExportQueueState;
struct IncrementalFence;

struct CommandQueueState {
    /// Pop a new command list
    /// \return nullptr if failed
    ID3D12GraphicsCommandList* PopCommandList();

    /// Push a completed command list
    /// \param list must be created from this queue
    void PushCommandList(ID3D12GraphicsCommandList* list);

    /// Parent state
    DeviceState* parent{nullptr};

    /// Object
    ID3D12CommandQueue* object{nullptr};

    /// Creation description
    D3D12_COMMAND_QUEUE_DESC desc;

    /// Queue export state
    ShaderExportQueueState* exportState{nullptr};

    /// Shared command allocator
    ID3D12CommandAllocator* commandAllocator{nullptr};

    /// On demand command lists
    std::vector<ID3D12GraphicsCommandList*> commandLists;

    /// Shared fence
    IncrementalFence* sharedFence{nullptr};

    /// Unique ID
    uint64_t uid{0};
};
