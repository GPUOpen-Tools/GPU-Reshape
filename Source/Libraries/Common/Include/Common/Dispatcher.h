#pragma once

// Common
#include "DispatcherWorker.h"
#include "DispatcherJobPool.h"
#include "IComponent.h"

// Std
#include <list>
#include <algorithm>

/// Simple work dispatcher
class Dispatcher : public IComponent {
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

private:
    /// Shared pool
    DispatcherJobPool pool;

    /// Worker list
    std::list<DispatcherWorker> workers;
};
