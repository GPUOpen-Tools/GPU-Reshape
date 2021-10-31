#pragma once

// Common
#include "DispatcherWorker.h"
#include "DispatcherJobPool.h"

// Std
#include <list>
#include <algorithm>

/// Simple work dispatcher
class Dispatcher {
public:
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
    void Add(const DispatcherJob* jobs, uint32_t count) {
        pool.Add(jobs, count);
    }

    /// Add a job to the dispatcher
    /// \param job the job to be added
    void Add(const DispatcherJob& job) {
        pool.Add(&job, 1);
    }

private:
    /// Shared pool
    DispatcherJobPool pool;

    /// Worker list
    std::list<DispatcherWorker> workers;
};
