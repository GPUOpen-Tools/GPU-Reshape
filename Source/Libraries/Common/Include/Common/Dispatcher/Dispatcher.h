// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Common
#include "DispatcherWorker.h"
#include "DispatcherJobPool.h"
#include <Common/IComponent.h>

// Std
#include <list>
#include <algorithm>

/// Simple work dispatcher
class Dispatcher : public TComponent<Dispatcher> {
public:
    COMPONENT(Dispatcher);

    Dispatcher(uint32_t workerCount = 0) {
        // Automatic worker count?
        if (!workerCount) {
            workerCount = std::max(1u, std::thread::hardware_concurrency() / 2u);
        }

        // Create workers
        for (uint32_t i = 0; i < workerCount; i++) {
            workers.emplace_back(pool);
        }
    }

    ~Dispatcher() {
        // Send the abort signal
        pool.Abort();

        // Wait for all workers
        for (DispatcherWorker& worker : workers) {
            worker.Join();
        }
    }

    /// Add a set of jobs to the dispatcher
    /// \param jobs the jobs to submit
    /// \param count the number of jobs
    void AddBatch(const DispatcherJob* jobs, uint32_t count) {
        pool.Add(jobs, count);
    }

    /// Add a job to the dispatcher
    /// \param job the job to be added
    void Add(const DispatcherJob& job) {
        if (job.bucket) {
            job.bucket->Increment();
        }

        pool.Add(&job, 1);
    }

    /// Add a job to the dispatcher
    /// \param job the job to be added
    void Add(const Delegate<void(void* userData)>& delegate, void* data, DispatcherBucket* bucket = nullptr) {
        Add(DispatcherJob{
            .userData = data,
            .delegate = delegate,
            .bucket = bucket
        });
    }

    /// Broadcast a job to all workers
    /// \param job the job to be broadcasted
    void Broadcast(const DispatcherJob& job) {
        for (uint32_t i = 0; i < workers.size(); i++) {
            pool.Add(&job, 1);
        }
    }

    /// Get the number of workers
    uint32_t WorkerCount() const {
        return static_cast<uint32_t>(workers.size());
    }

private:
    /// Shared pool
    DispatcherJobPool pool;

    /// Worker list
    std::list<DispatcherWorker> workers;
};
