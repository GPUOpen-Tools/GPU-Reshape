#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Backend
#include <Backend/Scheduler/IScheduler.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>
#include <mutex>
#include <Backends/Vulkan/States/ExclusiveQueue.h>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportStreamState;
struct QueueState;

class Scheduler final : public IScheduler {
public:
    /// Constructor
    /// \param table parent table
    explicit Scheduler(DeviceDispatchTable* table);

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
    /// Install a given queue
    /// \param queue the queue type
    /// \param exclusiveQueue the exclusive queue
    void InstallQueue(Queue queue, ExclusiveQueue exclusiveQueue);

private:
    struct Submission {
        /// Immediate command buffer
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};

        /// The streaming state
        ShaderExportStreamState* streamState{nullptr};

        /// The submission fence
        VkFence fence{VK_NULL_HANDLE};
    };

    struct QueueBucket {
        QueueBucket(const Allocators& allocators) : pendingSubmissions(allocators), freeSubmissions(allocators) {

        }

        /// Underlying queue
        VkQueue queue{VK_NULL_HANDLE};

        /// Shared pool for all submissions
        VkCommandPool pool{VK_NULL_HANDLE};

        /// All pending submissions
        Vector<Submission> pendingSubmissions;

        /// All free submissions
        Vector<Submission> freeSubmissions;
    };

    /// All queues
    Vector<QueueBucket> queues;

private:
    /// Pop a submission
    /// \param queue expected queue type
    /// \return submission object
    Submission PopSubmission(Queue queue);

private:
    /// Parent device
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;
};
