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
#include <Backends/DX12/DX12.h>

// Backend
#include <Backend/Scheduler/IScheduler.h>

// Common
#include <Common/Allocator/Vector.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>
#include <Backends/DX12/IncrementalFence.h>

// Forward declarations
class ShaderDataHost;
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

    /// Get the fence of a primitive
    /// \param pid primitive id
    /// \return native fence
    ID3D12Fence* GetPrimitiveFence(SchedulerPrimitiveID pid);

    /// Overrides
    void WaitForPending() override;
    void Schedule(Queue queue, const CommandBuffer &buffer, const SchedulerPrimitiveEvent* event) override;
    void MapTiles(Queue queue, ShaderDataID id, uint32_t count, const SchedulerTileMapping* mappings) override;
    SchedulerPrimitiveID CreatePrimitive() override;
    void DestroyPrimitive(SchedulerPrimitiveID pid) override;

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
    struct PrimitiveEntry {
        /// Underlying fence object
        ID3D12Fence* fence{nullptr};
    };

    /// All free fence indices
    Vector<uint32_t> freePrimitives;

    /// All primitives, sparsely laid out
    Vector<PrimitiveEntry> primitives;

private:
    /// Pop or construct a submission
    /// \param queue expected queue
    /// \return submission object
    Submission PopSubmission(Queue queue);

private:
    /// Parent device
    DeviceState* device;

    /// Shader data host component
    ComRef<ShaderDataHost> shaderDataHost;
    
    /// Shared lock
    std::mutex mutex;
};
