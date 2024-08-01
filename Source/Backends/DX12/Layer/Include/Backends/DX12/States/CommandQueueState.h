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
#include <Backends/DX12/States/ImmediateCommandList.h>
#include <Backends/DX12/States/CommandQueueExecutor.h>

// Common
#include <Common/Allocators.h>
#include <Common/Containers/Vector.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct ShaderExportQueueState;
struct IncrementalFence;

struct __declspec(uuid("0808310A-9E0B-41B6-81E5-4840CDF1EDAA")) CommandQueueState {
    CommandQueueState(const Allocators& allocators) : commandLists(allocators) {
        
    }
    
    ~CommandQueueState();

    /// Pop a new command list
    /// \return nullptr if failed
    ImmediateCommandList PopCommandList();

    /// Push a completed command list
    /// \param list must be created from this queue
    void PushCommandList(const ImmediateCommandList& list);

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Object
    ID3D12CommandQueue* object{nullptr};

    /// Creation description
    D3D12_COMMAND_QUEUE_DESC desc;

    /// Queue export state
    ShaderExportQueueState* exportState{nullptr};

    /// On demand command lists
    Vector<ImmediateCommandList> commandLists;

    /// Shared executor
    CommandQueueExecutor executor;

    /// Shared fence
    IncrementalFence* sharedFence{nullptr};

    /// Shared lock
    std::mutex mutex;

    /// Unique ID
    uint64_t uid{0};
};
