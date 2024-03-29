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
#include <Backends/DX12/DX12.h>

// Std
#include <cstdint>

struct IncrementalFence {
    ~IncrementalFence();

    /// Install this incremental fence
    /// \param device parent device
    /// \param queue parent queue
    /// \return success state
    bool Install(ID3D12Device* device, ID3D12CommandQueue* queue);

    /// Commit a new value
    /// \return CPU commit id
    uint64_t CommitFence();

    /// Check if a head has been commited
    /// \param head GPU head
    /// \return true if committed
    bool IsCommitted(uint64_t head);

    /// Parent queue
    ID3D12CommandQueue* queue;

    /// Shared fence
    ID3D12Fence* fence{nullptr};

    /// Commit heads
    uint64_t fenceCPUCommitId = 0x0;
    uint64_t fenceGPUCommitId = 0x0;
};
