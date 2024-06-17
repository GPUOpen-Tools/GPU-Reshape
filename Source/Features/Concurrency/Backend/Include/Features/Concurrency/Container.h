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

// Feature
#include <Features/Concurrency/Config.h>
#include <Features/Concurrency/FailureCode.h>

// Addressing
#include <Addressing/TexelMemoryAllocation.h>

// Backend
#include <Backend/Resource/ResourceInfo.h>

// Std
#include <unordered_map>
#include <mutex>

struct ConcurrencyContainer {
    struct Allocation {
#if CONCURRENCY_ENABLE_VALIDATION
        /// Debugging resource info
        ResourceInfo info;
#endif // CONCURRENCY_ENABLE_VALIDATION
        
        /// The underlying allocation
        TexelMemoryAllocation memory;

        /// Assigned initial failure code
        FailureCode failureCode{FailureCode::None};
    };

    /// All allocations
    std::unordered_map<uint64_t, Allocation> allocations;
    
    /// Shared lock
    std::mutex mutex;
};
