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
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("C36CADAF-D6C8-4DC4-B906-CCE432A96956")) FenceState {
    ~FenceState();

    /// Check if a commit has been completed
    /// \param commit the commit to check for
    /// \return true if completed
    bool IsCommitted(uint64_t commit) {
        // Check last known commit id
        if (cpuSignalCommitId >= commit) {
            return true;
        }

        // Query the gpu commit id
        return GetLatestCommit() >= commit;
    }

    /// Get the latest GPU commit id
    /// \return the
    uint64_t GetLatestCommit();

    /// Get the next to be signalled state
    /// \return
    uint64_t GetNextCommitID() const {
        return cpuSignalCommitId + 1;
    }

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Fence object
    ID3D12Fence* object{nullptr};

    /// Last completed GPU value
    uint64_t lastCompletedValue{0};

    /// Current CPU commit id, i.e. the currently known commit id
    uint64_t cpuSignalCommitId{0};

    /// Current signalling state
    bool signallingState{false};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
