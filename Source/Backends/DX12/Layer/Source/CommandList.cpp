#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/CommandAllocatorState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>

HRESULT HookID3D12DeviceCreateCommandQueue(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC *desc, const IID& riid, void **pCommandQueue) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandQueue *commandQueue{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandQueue(table.next, desc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void **>(&commandQueue));
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
        hr = commandQueue->QueryInterface(riid, pCommandQueue);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandQueue->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateCommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type, const IID& riid, void **pCommandAllocator) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandAllocator *commandAllocator{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandAllocator(table.next, type, __uuidof(ID3D12CommandAllocator), reinterpret_cast<void **>(&commandAllocator));
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
        hr = commandAllocator->QueryInterface(riid, pCommandAllocator);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandAllocator->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID& riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, type, Next(allocator), Next(initialState), __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new CommandListState();
    state->parent = table.state;

    // Create detours
    commandList = CreateDetour(Allocators{}, commandList, state);

    // Query to external object if requested
    if (pCommandList) {
        hr = commandList->QueryInterface(riid, pCommandList);
        if (FAILED(hr)) {
            return hr;
        }

        // Pass down the controller
        table.state->instrumentationController->BeginCommandList();
    }

    // Cleanup
    commandList->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList* list, ID3D12CommandAllocator* allocator, ID3D12PipelineState* state) {
    auto table = GetTable(list);

    // Pass down the controller
    table.state->parent->instrumentationController->BeginCommandList();

    // Pass down callchain
    HRESULT result = table.bottom->next_Reset(table.next, Next(allocator), Next(state));
    if (FAILED(result)) {
        return result;
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList* list) {
    auto table = GetTable(list);

    // Pass down callchain
    return table.bottom->next_Close(table.next);
}

void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList* list, ID3D12PipelineState* pipeline) {
    auto table = GetTable(list);

    // Pass down callchain
    return table.bottom->next_SetPipelineState(table.next, Next(pipeline));
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
