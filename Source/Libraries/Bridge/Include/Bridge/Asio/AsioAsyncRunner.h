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
