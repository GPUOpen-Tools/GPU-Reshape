#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Backend
#include <Backend/Scheduler/IScheduler.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>
#include <mutex>
#include <Backends/DX12/IncrementalFence.h>

// Forward declarations
struct DeviceState;
struct ShaderExportStreamState;

class Scheduler final : public IScheduler {
public:
    /// Constructor
    /// \param table parent table
    explicit Scheduler(DeviceState* table);

    /// Destructor
    ~Scheduler();

    /// Install this host
    /// \return success state
    bool Install();

    /// Invoke a synchronization point
    void SyncPoint();

    /// Wait for all pending submissions
    void WaitForPending();

    /// Overrides
    void Schedule(Queue queue, const CommandBuffer &buffer) override;

private:
    struct Submission {
        /// Immediate list
        ID3D12GraphicsCommandList* commandList{nullptr};

        /// Immediate allocator, tied to list
        ID3D12CommandAllocator* allocator{nullptr};

        /// Internal streaming state
        ShaderExportStreamState* streamState{nullptr};

        /// Internal fence for submission tracking
        IncrementalFence* fence{nullptr};

        /// Last known commit id
        uint64_t fenceCommitID{0};
    };

    struct QueueBucket {
        QueueBucket(const Allocators& allocators) : pendingSubmissions(allocators), freeSubmissions(allocators) {

        }

        /// Underlying queue object
        ID3D12CommandQueue* queue{nullptr};

        /// All pending submissions
        Vector<Submission> pendingSubmissions;

        /// All free submissions
        Vector<Submission> freeSubmissions;
    };

    /// All queues
    Vector<QueueBucket> queues;

private:
    /// Pop or construct a submission
    /// \param queue expected queue
    /// \return submission object
    Submission PopSubmission(Queue queue);

private:
    /// Parent device
    DeviceState* device;

    /// Shared lock
    std::mutex mutex;
};
