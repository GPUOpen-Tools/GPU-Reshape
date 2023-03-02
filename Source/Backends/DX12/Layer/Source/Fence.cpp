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
