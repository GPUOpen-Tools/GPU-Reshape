// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Std
#include <vector>
#include <chrono>

struct __declspec(uuid("664ECE39-6CD9-49E1-9790-21464F3F450A")) SwapChainState {
    SwapChainState(const Allocators& allocators) : buffers(allocators) {
        
    }
    
    ~SwapChainState();

    /// Target device
    ID3D12Device* device{};

    /// Parent factory
    IDXGIFactory* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Parent object
    IDXGISwapChain* object{};

    /// Last present time
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPresentTime = std::chrono::high_resolution_clock::now();

    /// Wrapped buffers
    Vector<ID3D12Resource*> buffers;
};
