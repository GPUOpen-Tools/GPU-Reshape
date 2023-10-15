// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Common
#include <Common/Dispatcher/Mutex.h>
#include <Common/Dispatcher/ConditionVariable.h>

struct EventCounter {
    /// Wait until a given value is satisfied
    void Wait(uint64_t value);

    /// Increment the head (the value to be reached)
    void IncrementHead(uint64_t value = 1) {
        MutexGuard guard(mutex);
        head += value;
    }

    /// Increment the counter (the completion value)
    void IncrementCounter(uint64_t value = 1) {
        MutexGuard guard(mutex);
        counter += value;
        var.NotifyAll();
    }

    /// Reset this counter, not thread safe
    void Reset() {
        head = 0;
        counter = 0;
    }

    /// Get the current head value
    uint64_t GetHead() {
        MutexGuard guard(mutex);
        uint64_t value = head;

        return value;
    }

private:
    uint64_t          head{0};
    uint64_t          counter{0};
    Mutex             mutex;
    ConditionVariable var;
};
