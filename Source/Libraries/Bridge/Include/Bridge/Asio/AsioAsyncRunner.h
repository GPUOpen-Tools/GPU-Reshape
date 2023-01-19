#pragma once

// Std
#include <thread>

/// Simply async runner for asio tasks
template<typename T>
class AsioAsyncRunner {
public:
    AsioAsyncRunner() = default;

    /// Deconstructor, waits if possible
    ~AsioAsyncRunner() {
        Stop();
    }

    /// Run the worker asynchronously
    /// \param runner client to be run
    void RunAsync(T& runner) {
        std::lock_guard guard(mutex);

        // Already launched?
        if (!exitFlag.load()) {
            return;
        }

        // Remove exit flag, marks it as launched
        exitFlag = false;

        // Spin thread
        thread = std::thread([this, &runner]{
            Worker(runner);
        });
    }

    /// Stop the worker
    void Stop() {
        std::lock_guard guard(mutex);

        // Mark exit flag
        exitFlag = true;

        // Attempt to join
        if (thread.joinable()) {
            thread.join();
        }
    }
    
private:
    /// Worker thread
    /// \param runner client to be run
    void Worker(T& runner) {
        while (!exitFlag.load()) {
            runner.Run();

            // Release the thread, could yield instead
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    /// Exit flag
    std::atomic<bool> exitFlag{true};

    /// Shared lock
    std::mutex mutex;

    /// Worker thread
    std::thread thread;
};
