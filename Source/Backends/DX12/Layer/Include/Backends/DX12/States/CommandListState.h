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
#include <Backends/DX12/FeatureProxies.Gen.h>

// Backend
#include <Backend/CommandContext.h>

// Common
#include <Common/Allocators.h>
#include <Common/Containers/SlotArray.h>

// Forward declarations
struct DeviceState;
struct ShaderExportStreamState;
class DescriptorDataAppendAllocator;

struct __declspec(uuid("8270D898-4356-4503-8DEB-9CD73BB31B21")) CommandListState {
    ~CommandListState();

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Current streaming state
    ShaderExportStreamState* streamState{nullptr};

    /// The actual list type
    D3D12_COMMAND_LIST_TYPE userType = D3D12_COMMAND_LIST_TYPE_DIRECT;

    /// All contained proxies
    ID3D12GraphicsCommandListFeatureProxies proxies;

    /// User context
    CommandContext userContext;

    /// The allocator currently owning this command list
    ID3D12CommandAllocator* owningAllocator{nullptr};

    /// Allocator slot index
    SlotId allocatorSlotId{kInvalidSlotId};

    /// Object
    ID3D12GraphicsCommandList* object{nullptr};
};
