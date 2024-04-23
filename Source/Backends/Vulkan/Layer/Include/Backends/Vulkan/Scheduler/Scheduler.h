// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/ExclusiveQueue.h>

// Backend
#include <Backend/Scheduler/IScheduler.h>

// Common
#include <Common/Allocator/Vector.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportStreamState;
struct QueueState;
class DeviceAllocator;

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

    /// Get the underlying semaphore
    /// \param pid primitive id
    /// \return native object
    VkSemaphore GetPrimitiveSemaphore(SchedulerPrimitiveID pid);

    /// Overrides
    void WaitForPending() override;
    SchedulerPrimitiveID CreatePrimitive() override;
    void DestroyPrimitive(SchedulerPrimitiveID pid) override;
    void MapTiles(Queue queue, ShaderDataID id, uint32_t count, const SchedulerTileMapping* mappings) override;
    void Schedule(Queue queue, const CommandBuffer &buffer, const SchedulerPrimitiveEvent* event) override;

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
    struct PrimitiveEntry {
        /// Underlying semaphore
        VkSemaphore semaphore{VK_NULL_HANDLE};
    };

    /// All free primitive indices
    Vector<uint32_t> freePrimitives;

    /// All primitives, sparsely laid out
    Vector<PrimitiveEntry> primitives;

private:
    /// Parent device
    DeviceDispatchTable* table;

    /// Device memory allocator
    ComRef<DeviceAllocator> deviceAllocator;

    /// Shared lock
    std::mutex mutex;
};
