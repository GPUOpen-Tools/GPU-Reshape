#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/CommandAllocatorState.h>
#include <Backends/DX12/States/DeviceState.h>

HRESULT HookID3D12DeviceCreateCommandQueue(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC *desc, const IID *const riid, void **pCommandQueue) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandQueue *commandQueue{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandQueue(table.next, desc, &__uuidof(ID3D12CommandQueue), reinterpret_cast<void **>(&commandQueue));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new CommandQueueState();
    state->parent = table.state;

    // Create detours
    commandQueue = CreateDetour(Allocators{}, commandQueue, state);

    // Query to external object if requested
    if (pCommandQueue) {
        hr = commandQueue->QueryInterface(*riid, pCommandQueue);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandQueue->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateCommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type, const IID *const riid, void **pCommandAllocator) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandAllocator *commandAllocator{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandAllocator(table.next, type, &__uuidof(ID3D12CommandAllocator), reinterpret_cast<void **>(&commandAllocator));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new CommandAllocatorState();
    state->parent = table.state;

    // Create detours
    commandAllocator = CreateDetour(Allocators{}, commandAllocator, state);

    // Query to external object if requested
    if (pCommandAllocator) {
        hr = commandAllocator->QueryInterface(*riid, pCommandAllocator);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandAllocator->Release();

    // OK
    return S_OK;
}

static HRESULT CreateD3D12DeviceGraphicsCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID *const riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12GraphicsCommandList *commandList{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, type, Next(allocator), Next(initialState), &__uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new GraphicsCommandListState();
    state->parent = table.state;

    // Create detours
    commandList = CreateDetour(Allocators{}, commandList, state);

    // Query to external object if requested
    if (pCommandList) {
        hr = commandList->QueryInterface(*riid, pCommandList);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandList->Release();

    // OK
    return S_OK;
}

static HRESULT CreateD3D12DeviceComputeCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID *const riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, type, Next(allocator), Next(initialState), &__uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new ComputeCommandListState();
    state->parent = table.state;

    // Create detours
    commandList = CreateDetour(Allocators{}, commandList, state);

    // Query to external object if requested
    if (pCommandList) {
        hr = commandList->QueryInterface(*riid, pCommandList);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandList->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID *const riid, void **pCommandList) {
    switch (type) {
        default: {
            ASSERT(false, "Unsupported command list type");
            return S_FALSE;
        }
        case D3D12_COMMAND_LIST_TYPE_DIRECT: {
            return CreateD3D12DeviceGraphicsCreateCommandList(device, nodeMask, type, allocator, initialState, riid, pCommandList);
        }
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: {
            return CreateD3D12DeviceComputeCreateCommandList(device, nodeMask, type, allocator, initialState, riid, pCommandList);
        }
    }
}

void HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue *queue, UINT count, ID3D12CommandList *const * lists) {
    auto table = GetTable(queue);

    // Unwrap all lists
    auto* unwrapped = ALLOCA_ARRAY(ID3D12CommandList*, count);
    for (uint32_t i = 0; i < count; i++) {
        unwrapped[i] = Next(lists[i]);
    }

    // Pass down callchain
    table.bottom->next_ExecuteCommandLists(table.next, count, unwrapped);
}

ULONG WINAPI HookID3D12CommandQueueRelease(ID3D12CommandQueue *queue) {
    auto table = GetTable(queue);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}

ULONG WINAPI HookID3D12CommandAllocatorRelease(ID3D12CommandAllocator *allocator) {
    auto table = GetTable(allocator);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}

ULONG HookID3D12CommandListRelease(ID3D12CommandList *list) {
    auto table = GetTable(list);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}
ULONG HookID3D12GraphicsCommandListRelease(ID3D12GraphicsCommandList *list) {
    auto table = GetTable(list);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}
