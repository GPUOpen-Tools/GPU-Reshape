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
#include <chrono>
#include <atomic>
#include <functional>
#include <thread>

class IntervalActionThread {
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    
public:
    /// Constructor
    /// \param interval given interval, initial action excluded
    IntervalActionThread(const std::chrono::milliseconds &interval) : interval(interval) {
        
    }

    /// Destructor
    ~IntervalActionThread() {
        Stop();
    }

    /// Start this action thread
    /// \param action functor to be invoked on each interval
    void Start(const std::function<void()>& action) {
        this->action = action;
        thread = std::thread(&IntervalActionThread::ThreadEntry, this);
    }

    /// Stop this action thread
    void Stop() {
        exitFlag = true;

        // Join if possible
        if (thread.joinable()) {
            thread.join();
        }
    }
    
private:
    void ThreadEntry() {
        auto last = std::chrono::high_resolution_clock::now();

        // While open
        while (!exitFlag.load()) {
            action();

            // Record after the action, ensures we take the action time into account
            auto now = std::chrono::high_resolution_clock::now();

            // If the action time exceeds the interval, don't yield the thread
            auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
            if (deltaMs < interval) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval - deltaMs));
            }
    
            last = now;
        }
    }

public:
    /// Create an action thread from a millisecond interval
    /// \param count number of milliseconds
    static IntervalActionThread FromMS(size_t count) {
        return IntervalActionThread(std::chrono::milliseconds(count));
    }

private:
    /// Constant interval
    std::chrono::milliseconds interval;

    /// Owned thread
    std::thread thread;

    /// Action to be invoked each interval
    std::function<void()> action;

    /// Thread exit condition
    std::atomic<bool> exitFlag;
};
