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
        if (thread.joinable()) {
            thread.join();
        }
    }

    /// Run the worker asynchronously
    void RunAsync(T& runner) {
        thread = std::thread([this, &runner]{
            Worker(runner);
        });
    }

private:
    void Worker(T& runner) {
        runner.Run();
    }

private:
    std::thread thread;
};
