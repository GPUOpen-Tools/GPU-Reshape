// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Common
#include <Common/Assert.h>
#include "DispatcherBucket.h"
#include "DispatcherJobPool.h"

// Std
#include <thread>

/// Simple dispatcher worker
class DispatcherWorker {
public:
    DispatcherWorker(DispatcherJobPool& pool) : pool(pool) {
        thread = std::thread(&DispatcherWorker::ThreadEntry, this);
    }

    /// Attempt to join the thread
    void Join() {
        if (!thread.joinable()) {
            ASSERT(false, "Worker not joinable");
            return;
        }

        thread.join();
    }

private:
    void ThreadEntry() {
        for (;;) {
            DispatcherJob job;

            // Blocking pop, false indicates abort condition
            if (!pool.PopBlocking(job)) {
                return;
            }

            // Invoke
            job.delegate.Invoke(job.userData);

            // Invoke bucket functor if reached zero
            if (job.bucket) {
                job.bucket->Decrement();
            }
        }
    }

private:
    /// Worker thread
    std::thread thread;

    /// Shared pool
    DispatcherJobPool& pool;
};