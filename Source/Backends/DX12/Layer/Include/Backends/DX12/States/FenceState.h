#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("C36CADAF-D6C8-4DC4-B906-CCE432A96956")) FenceState {
    ~FenceState();

    /// Check if a commit has been completed
    /// \param commit the commit to check for
    /// \return true if completed
    bool IsCommitted(uint64_t commit) {
        // Check last known commit id
        if (cpuSignalCommitId >= commit) {
            return true;
        }

        // Query the gpu commit id
        return GetLatestCommit() >= commit;
    }

    /// Get the latest GPU commit id
    /// \return the
    uint64_t GetLatestCommit();

    /// Get the next to be signalled state
    /// \return
    uint64_t GetNextCommitID() const {
        return cpuSignalCommitId + 1;
    }

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Fence object
    ID3D12Fence* object{nullptr};

    /// Last completed GPU value
    uint64_t lastCompletedValue{0};

    /// Current CPU commit id, i.e. the currently known commit id
    uint64_t cpuSignalCommitId{0};

    /// Current signalling state
    bool signallingState{false};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
