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

// Layer
#include <Backend/IL/ResourceTokenPacking.h>

// Common
#include <Common/Assert.h>

// Std
#include <mutex>
#include <vector>

struct PhysicalResourceIdentifierMap {
    /// Allocate a new PUID
    /// \return
    uint32_t AllocatePUID() {
        std::lock_guard guard(mutex);
        
        if (freePUIDs.empty()) {
            ASSERT(puidHead < IL::kResourceTokenPUIDInvalidStart, "Exceeded maximum resource count");
            return puidHead++;
        }

        uint32_t puid = freePUIDs.back();
        freePUIDs.pop_back();
        return puid;
    }

    /// Free a puid
    /// \param puid
    void FreePUID(uint32_t puid) {
        std::lock_guard guard(mutex);
        freePUIDs.push_back(puid);
    }

private:
    /// Shared lock
    std::mutex mutex;
    
    /// Current head counter
    uint32_t puidHead{IL::kResourceTokenPUIDReservedCount};

    /// All free indices
    std::vector<uint32_t> freePUIDs;
};
