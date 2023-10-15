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

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Resource/VirtualResourceMapping.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("09175D5B-BA8A-4531-9553-BC1CD024A1FE")) ResourceState {
    ~ResourceState();

    /// Parent state
    ID3D12Device* parent{};

    /// User object
    ID3D12Resource* object{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Resource creation
    D3D12_RESOURCE_DESC desc;

    /// Optional debug name
    char* debugName{nullptr};

    /// Resource mapping
    VirtualResourceMapping virtualMapping;

    /// Unique ID
    uint64_t uid{0};
};
