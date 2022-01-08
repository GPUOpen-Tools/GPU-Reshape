#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>

// Forward declarations
struct DeviceDispatchTable;

struct FenceState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~FenceState();

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

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkFence object{VK_NULL_HANDLE};

    /// Current CPU commit id, i.e. the currently known commit id
    uint64_t cpuSignalCommitId{0};

    /// Current signalling state
    bool signallingState{false};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
