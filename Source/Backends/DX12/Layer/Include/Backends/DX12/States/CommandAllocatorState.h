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
#include <Backends/DX12/States/CommandListState.h>

// Common
#include <Common/Allocators.h>
#include <Common/Containers/SlotArray.h>

// Std
#include <mutex>

struct __declspec(uuid("23000608-5CA5-4865-883A-4A864750B14B")) CommandAllocatorState {
    ~CommandAllocatorState();

    /// Parent state
    ID3D12Device* parent{};

    /// User command list type
    D3D12_COMMAND_LIST_TYPE userType = D3D12_COMMAND_LIST_TYPE_DIRECT;

    /// All streaming states tracked by this allocator
    std::vector<ShaderExportStreamState*> streamStates;

    /// All command lists currently allocating from this allocator
    SlotArray<CommandListState*, &CommandListState::allocatorSlotId> commandLists;

    /// Shared lock for thread safe usage
    std::mutex lock;

    /// Owning allocator
    Allocators allocators;
};
