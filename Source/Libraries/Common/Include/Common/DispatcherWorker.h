#pragma once

// Common
#include "Assert.h"
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
        }
    }

private:
    /// Worker thread
    std::thread thread;

    /// Shared pool
    DispatcherJobPool& pool;
};