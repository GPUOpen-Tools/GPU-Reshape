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
        var.NotifyAll();
    }

    /// Pop a job from the pool
    /// \param out the popped job, if succeeded
    /// \return success
    bool Pop(DispatcherJob& out) {
        MutexGuard guard(mutex);

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

    /// Is this pool aborted?
    [[nodiscard]]
    bool IsAbort() const {
        return abortFlag;
    }

private:
    /// Pool lock
    Mutex mutex;

    /// Exit flag for the pool
    bool abortFlag{false};

    /// Shared var for waits
    ConditionVariable var;

    /// Enqueued jobs
    std::vector<DispatcherJob> pool;
};
