// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/DX12/Fence.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/FenceState.h>
#include <Backends/DX12/States/DeviceState.h>

HRESULT HookID3D12DeviceCreateFence(ID3D12Device* device, UINT64 nodeMask, D3D12_FENCE_FLAGS flags, const IID& riid, void** pFence) {
    auto table = GetTable(device);

    // Object
    ID3D12Fence* fence{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateFence(table.next, nodeMask, flags, __uuidof(ID3D12Fence), reinterpret_cast<void**>(&fence));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) FenceState();
    state->allocators = table.state->allocators;
    state->parent = device;

    // Create detours
    fence = CreateDetour(state->allocators, fence, state);

    // Query to external object if requested
    if (pFence) {
        hr = fence->QueryInterface(riid, pFence);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    fence->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12FenceGetDevice(ID3D12Fence *_this, const IID &riid, void **ppDevie) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevie);
}

FenceState::~FenceState() {

}

uint64_t FenceState::GetLatestCommit() {
    // Pass down callchain
    uint64_t completedValue = object->GetCompletedValue();

    // If not signalled yet, and fence is done, advance the commit
    if (!signallingState || lastCompletedValue != completedValue) {
        signallingState = true;
        cpuSignalCommitId++;
    }

    // Set known last
    lastCompletedValue = completedValue;

    // Return new commit
    return cpuSignalCommitId;
}

HRESULT WINAPI HookID3D12DeviceSetEventOnMultipleFenceCompletion(ID3D12Device* _this, ID3D12Fence* const* ppFences, const UINT64* pFenceValues, UINT NumFences, D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags, HANDLE hEvent) {
    auto table = GetTable(_this);

    // Unwrap all objects
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Fence*, NumFences);
    for (uint32_t i = 0; i < NumFences; i++) {
        unwrapped[i] = Next(ppFences[i]);
    }

    // Pass down call chain
    return table.next->SetEventOnMultipleFenceCompletion(unwrapped, pFenceValues, NumFences, Flags, hEvent);
}
