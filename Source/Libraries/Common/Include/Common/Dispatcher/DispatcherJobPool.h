#pragma once

// Common
#include "DispatcherJob.h"

// Std
#include <vector>
#include <condition_variable>

/// Job pool for all dispatcher jobs
struct DispatcherJobPool {
    /// Add a set of jobs to the pool
    /// \param jobs the jobs to submit
    /// \param count the number of jobs
    void Add(const DispatcherJob* jobs, uint32_t count) {
        std::lock_guard guard(mutex);
        pool.insert(pool.end(), jobs, jobs + count);
        var.notify_all();
    }

    /// Pop a job from the pool
    /// \param out the popped job, if succeeded
    /// \return success
    bool Pop(DispatcherJob& out) {
        std::lock_guard guard(mutex);

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
    bool PopBlocking(DispatcherJob& out) {
        // Wait for item or abort signal
        std::unique_lock lock(mutex);
        var.wait(lock, [this] {
            return !pool.empty() || abortFlag;
        });

        // Abort?
        if (abortFlag) {
            return false;
        }

        // Pop back
        out = pool.back();
        pool.pop_back();
        return true;
    }

    /// Set the abort flag
    void Abort() {
        std::lock_guard guard(mutex);
        abortFlag = true;

        // Wake all threads
        var.notify_all();
    }

    /// Is this pool aborted?
    [[nodiscard]]
    bool IsAbort() const {
        return abortFlag;
    }

private:
    /// Pool lock
    std::mutex mutex;

    /// Exit flag for the pool
    bool abortFlag{false};

    /// Shared var for waits
    std::condition_variable var;

    /// Enqueued jobs
    std::vector<DispatcherJob> pool;
};
