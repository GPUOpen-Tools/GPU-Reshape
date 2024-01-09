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

class IntervalAction {
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    
public:
    /// Constructor
    /// \param interval given interval, initial action excluded 
    IntervalAction(const std::chrono::milliseconds &interval) : interval(interval) {
        lastEvent = std::chrono::high_resolution_clock::now();
    }

    /// Step this action
    /// \return true if the inverval has passed
    bool Step() {
        TimePoint now = std::chrono::high_resolution_clock::now();

        // Pending interval?
        if (now - lastEvent < interval) {
            return false;
        }

        // Set last event
        lastEvent = now;
        return true;
    }

public:
    /// Create an action from a millisecond interval
    /// \param count number of milliseconds
    static IntervalAction FromMS(size_t count) {
        return IntervalAction(std::chrono::milliseconds(count));
    }

private:
    /// Last time this action was triggered
    TimePoint lastEvent;

    /// Constant interval
    std::chrono::milliseconds interval;
};
