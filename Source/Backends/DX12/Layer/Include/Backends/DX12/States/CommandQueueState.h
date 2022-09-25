#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "ImmediateCommandList.h"

// Common
#include <Common/Allocators.h>

// Std
#include <vector>

// Forward declarations
struct ShaderExportQueueState;
struct IncrementalFence;

struct CommandQueueState {
    ~CommandQueueState();

    /// Pop a new command list
    /// \return nullptr if failed
    ImmediateCommandList PopCommandList();

    /// Push a completed command list
    /// \param list must be created from this queue
    void PushCommandList(const ImmediateCommandList& list);

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Object
    ID3D12CommandQueue* object{nullptr};

    /// Creation description
    D3D12_COMMAND_QUEUE_DESC desc;

    /// Queue export state
    ShaderExportQueueState* exportState{nullptr};

    /// On demand command lists
    std::vector<ImmediateCommandList> commandLists;

    /// Shared fence
    IncrementalFence* sharedFence{nullptr};

    /// Unique ID
    uint64_t uid{0};
};
