#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Std
#include <cstdint>

struct IncrementalFence {
    ~IncrementalFence();

    /// Install this incremental fence
    /// \param device parent device
    /// \param queue parent queue
    /// \return success state
    bool Install(ID3D12Device* device, ID3D12CommandQueue* queue);

    /// Commit a new value
    /// \return CPU commit id
    uint64_t CommitFence();

    /// Check if a head has been commited
    /// \param head GPU head
    /// \return true if committed
    bool IsCommitted(uint64_t head);

    /// Parent queue
    ID3D12CommandQueue* queue;

    /// Shared fence
    ID3D12Fence* fence{nullptr};

    /// Commit heads
    uint64_t fenceCPUCommitId = 0x0;
    uint64_t fenceGPUCommitId = 0x0;
};
