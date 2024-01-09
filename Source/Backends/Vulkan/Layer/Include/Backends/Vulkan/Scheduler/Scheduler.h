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

    /// Overrides
    void WaitForPending() override;
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
