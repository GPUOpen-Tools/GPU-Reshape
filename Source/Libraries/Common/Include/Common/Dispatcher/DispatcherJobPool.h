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

// Common
#include "DispatcherJob.h"
#include "Mutex.h"
#include "ConditionVariable.h"

// Std
#include <vector>

/// Job pool for all dispatcher jobs
struct DispatcherJobPool {
    /// Add a set of jobs to the pool
    /// \param jobs the jobs to submit
    /// \param count the number of jobs
    void Add(const DispatcherJob* jobs, uint32_t count) {
        MutexGuard guard(mutex);
        pool.insert(pool.end(), jobs, jobs + count);

        // Notify if not paused
        if (!pauseFlag) {
            var.NotifyAll();
        }
    }

    /// Pop a job from the pool
    /// \param out the popped job, if succeeded
    /// \return success
    bool Pop(DispatcherJob& out) {
        MutexGuard guard(mutex);

        // If paused, pretend there's nothing
        if (pauseFlag) {
            return false;
        }

        // Any jobs?
        if (pool.empty()) {
            return false;
        }

        // Pop from back
        out = pool.back();
        pool.pop_back();
        return true;
    }

    /// Perform a blocking wait for a job
    /// \param out the job
    /// \return false if abort has been signalled
    bool PopBlocking(DispatcherJob& out);

    /// Set the abort flag
    void Abort() {
        MutexGuard guard(mutex);
        abortFlag = true;

        // Wake all threads
        var.NotifyAll();
    }

    /// Set the new paused state
    /// \param paused if false, wakes all threads
    void SetPaused(bool paused) {
        MutexGuard guard(mutex);
        pauseFlag = paused;

        // Wake all threads if resumed
        if (!paused) {
            var.NotifyAll();
        }
    }

    /// Is this pool aborted?
    bool IsAbort() const {
        return abortFlag;
    }

    /// Is this pool currently paused?
    bool IsPaused() const {
        return pauseFlag;
    }

private:
    /// Pool lock
    Mutex mutex;

    /// Exit flag for the pool
    bool abortFlag{false};

    /// Pause flag for the pool
    bool pauseFlag{false};

    /// Shared var for waits
    ConditionVariable var;

    /// Enqueued jobs
    std::vector<DispatcherJob> pool;
};
