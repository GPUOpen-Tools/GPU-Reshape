#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/CommandAllocatorState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/IncrementalFence.h>

HRESULT HookID3D12DeviceCreateCommandQueue(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC *desc, const IID &riid, void **pCommandQueue) {
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
    state->allocators = table.state->allocators;
    state->parent = table.state;
    state->desc = *desc;
    state->object = commandQueue;

    // Create detours
    commandQueue = CreateDetour(Allocators{}, commandQueue, state);

    // Query to external object if requested
    if (pCommandQueue) {
        hr = commandQueue->QueryInterface(riid, pCommandQueue);
        if (FAILED(hr)) {
            return hr;
        }

        // Create shared allocator
        hr = table.next->CreateCommandAllocator(desc->Type, IID_PPV_ARGS(&state->commandAllocator));
        if (FAILED(hr)) {
            return hr;
        }

        // Create shared fence
        state->sharedFence = new (table.state->allocators) IncrementalFence();
        if (!state->sharedFence->Install(table.next, state->object)) {
            return E_FAIL;
        }

        // Create export state
        state->exportState = table.state->exportStreamer->AllocateQueueState(commandQueue);

        // Add state
        table.state->states_Queues.Add(state);
    }

    // Cleanup
    commandQueue->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue* queue, ID3D12Fence* pFence, UINT64 Value) {
    auto table = GetTable(queue);

    // Pass down callchain
    return table.bottom->next_Signal(table.next, Next(pFence), Value);
}

HRESULT WINAPI HookID3D12CommandQueueWait(ID3D12CommandQueue* queue, ID3D12Fence* pFence, UINT64 Value) {
    auto table = GetTable(queue);

    // Pass down callchain
    return table.bottom->next_Wait(table.next, Next(pFence), Value);
}

HRESULT HookID3D12DeviceCreateCommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type, const IID &riid, void **pCommandAllocator) {
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
    state->allocators = table.state->allocators;
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

static ID3D12PipelineState *GetHotSwapPipeline(ID3D12PipelineState *initialState) {
    if (!initialState) {
        return nullptr;
    }

    // Available hot swap?
    if (ID3D12PipelineState *hotSwap = GetState(initialState)->hotSwapObject.load()) {
        return hotSwap;
    }

    return nullptr;
}

HRESULT HookID3D12DeviceCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID &riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(initialState);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, type, Next(allocator), hotSwap ? hotSwap : Next(initialState), __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new CommandListState();
    state->allocators = table.state->allocators;
    state->parent = table.state;
    state->object = static_cast<ID3D12GraphicsCommandList*>(commandList);

    // Create detours
    commandList = CreateDetour(Allocators{}, commandList, state);

    // Query to external object if requested
    if (pCommandList) {
        hr = commandList->QueryInterface(riid, pCommandList);
        if (FAILED(hr)) {
            return hr;
        }

        // Create export state
        state->streamState = table.state->exportStreamer->AllocateStreamState();

        // Inform the streamer
        table.state->exportStreamer->BeginCommandList(state->streamState, state);

        // Inform the streamer of a new pipeline
        if (initialState) {
            table.state->exportStreamer->BindPipeline(state->streamState, GetState(initialState), hotSwap != nullptr, state);
        }

        // Pass down the controller
        table.state->instrumentationController->BeginCommandList();
    }

    // Cleanup
    commandList->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList *list, ID3D12CommandAllocator *allocator, ID3D12PipelineState *state) {
    auto table = GetTable(list);

    // Pass down the controller
    table.state->parent->instrumentationController->BeginCommandList();

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(state);

    // Pass down callchain
    HRESULT result = table.bottom->next_Reset(table.next, Next(allocator), hotSwap ? hotSwap : Next(state));
    if (FAILED(result)) {
        return result;
    }

    // Inform the streamer
    table.state->parent->exportStreamer->BeginCommandList(table.state->streamState, table.state);

    // Inform the streamer
    if (state) {
        table.state->parent->exportStreamer->BindPipeline(table.state->streamState, GetState(state), hotSwap != nullptr, table.state);
    }

    // OK
    return S_OK;
}

void WINAPI HookID3D12CommandListSetDescriptorHeaps(ID3D12CommandList* list, UINT NumDescriptorHeaps, ID3D12DescriptorHeap *const *ppDescriptorHeaps) {
    auto table = GetTable(list);

    // Allocate unwrapped
    auto *unwrapped = ALLOCA_ARRAY(ID3D12DescriptorHeap*, NumDescriptorHeaps);

    // Unwrap
    for (uint32_t i = 0; i < NumDescriptorHeaps; i++) {
        auto heapTable = GetTable(ppDescriptorHeaps[i]);
        unwrapped[i] = heapTable.next;
    }

    // Pass down callchain
    table.bottom->next_SetDescriptorHeaps(table.next, NumDescriptorHeaps, unwrapped);

    // Let the streamer handle allocations
    for (uint32_t i = 0; i < NumDescriptorHeaps; i++) {
        auto heapTable = GetTable(ppDescriptorHeaps[i]);
        table.state->parent->exportStreamer->SetDescriptorHeap(table.state->streamState, heapTable.state, table.state);
    }
}


void WINAPI HookID3D12CommandListCopyTextureRegion(ID3D12CommandList* list, const D3D12_TEXTURE_COPY_LOCATION* pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox) {
    auto table = GetTable(list);

    // Unwrap src
    D3D12_TEXTURE_COPY_LOCATION src = *pSrc;
    src.pResource = Next(src.pResource);

    // Unwrap dst
    D3D12_TEXTURE_COPY_LOCATION dst = *pDst;
    dst.pResource = Next(dst.pResource);

    // Pass down callchain
    table.bottom->next_CopyTextureRegion(table.next, &dst, DstX, DstY, DstZ, &src, pSrcBox);
}

void WINAPI HookID3D12CommandListResourceBarrier(ID3D12CommandList* list, UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers) {
    auto table = GetTable(list);

    // Copy wrapper barriers
    auto barriers = ALLOCA_ARRAY(D3D12_RESOURCE_BARRIER, NumBarriers);
    std::memcpy(barriers, pBarriers, sizeof(D3D12_RESOURCE_BARRIER) * NumBarriers);

    // Unwrap all objects
    for (uint32_t i = 0; i < NumBarriers; i++) {
        switch (barriers[i].Type) {
            case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION: {
                barriers[i].Transition.pResource = Next(barriers[i].Transition.pResource);
                break;
            }
            case D3D12_RESOURCE_BARRIER_TYPE_ALIASING: {
                if (barriers[i].Aliasing.pResourceBefore) {
                    barriers[i].Aliasing.pResourceBefore = Next(barriers[i].Aliasing.pResourceBefore);
                }
                if (barriers[i].Aliasing.pResourceAfter) {
                    barriers[i].Aliasing.pResourceAfter = Next(barriers[i].Aliasing.pResourceAfter);
                }
                break;
            }
            case D3D12_RESOURCE_BARRIER_TYPE_UAV: {
                if (barriers[i].UAV.pResource) {
                    barriers[i].UAV.pResource = Next(barriers[i].Transition.pResource);
                }
                break;
            }
        }
    }

    // Pass down callchain
    table.bottom->next_ResourceBarrier(table.next, NumBarriers, barriers);
}

void WINAPI HookID3D12CommandListSetGraphicsRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature) {
    auto table = GetTable(list);
    auto rsTable = GetTable(rootSignature);

    // Pass down callchain
    table.bottom->next_SetGraphicsRootSignature(table.next, rsTable.next);

    // Inform the streamer of a new root signature
    table.state->parent->exportStreamer->SetRootSignature(table.state->streamState, rsTable.state, table.state);
}

void WINAPI HookID3D12CommandListSetComputeRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature) {
    auto table = GetTable(list);
    auto rsTable = GetTable(rootSignature);

    // Pass down callchain
    table.bottom->next_SetComputeRootSignature(table.next, rsTable.next);

    // Inform the streamer of a new root signature
    table.state->parent->exportStreamer->SetRootSignature(table.state->streamState, rsTable.state, table.state);
}

HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList *list) {
    auto table = GetTable(list);

    // Pass down callchain
    return table.bottom->next_Close(table.next);
}

void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList *list, ID3D12PipelineState *pipeline) {
    auto table = GetTable(list);

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(pipeline);

    // Pass down callchain
    table.bottom->next_SetPipelineState(table.next, hotSwap ? hotSwap : Next(pipeline));

    // Inform the streamer of a new pipeline
    table.state->parent->exportStreamer->BindPipeline(table.state->streamState, GetState(pipeline), hotSwap != nullptr, table.state);
}

AGSReturnCode HookAMDAGSDestroyDevice(AGSContext* context, ID3D12Device* device, unsigned int* deviceReferences) {
    return D3D12GPUOpenFunctionTableNext.next_AMDAGSDestroyDevice(context, Next(device), deviceReferences);
}

AGSReturnCode HookAMDAGSPushMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data) {
    return D3D12GPUOpenFunctionTableNext.next_AMDAGSPushMarker(context, Next(commandList), data);
}

AGSReturnCode HookAMDAGSPopMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList) {
    return D3D12GPUOpenFunctionTableNext.next_AMDAGSPopMarker(context, Next(commandList));
}

AGSReturnCode HookAMDAGSSetMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data) {
    return D3D12GPUOpenFunctionTableNext.next_AMDAGSSetMarker(context, Next(commandList), data);
}

void HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue *queue, UINT count, ID3D12CommandList *const *lists) {
    auto table = GetTable(queue);

    // Parent device
    DeviceState* device = table.state->parent;

    // Process any remaining work on the queue
    device->exportStreamer->Process(table.state);

    // Special case, invoke a device sync point during empty submissions
    if (count == 0) {
        BridgeDeviceSyncPoint(device);
    }

    // Allocate submission segment
    ShaderExportStreamSegment* segment = device->exportStreamer->AllocateSegment();

    // Allocate unwrapped, +1 for patch
    auto *unwrapped = ALLOCA_ARRAY(ID3D12CommandList*, count + 1);

    // Process all lists
    for (uint32_t i = 0; i < count; i++) {
        auto listTable = GetTable(lists[i]);

        // Create streamer allocation association
        device->exportStreamer->MapSegment(listTable.state->streamState, segment);

        // Pass down unwrapped
        unwrapped[i] = listTable.next;
    }

    // Record the streaming patching
    unwrapped[count] = device->exportStreamer->RecordPatchCommandList(table.state, segment);

    // Pass down callchain
    table.bottom->next_ExecuteCommandLists(table.next, count + 1, unwrapped);

    // Notify streamer of submission
    device->exportStreamer->Enqueue(table.state, segment);
}

CommandQueueState::~CommandQueueState() {
    // Clean the streaming state for this queue
    parent->exportStreamer->Process(this);

    // Remove state
    parent->states_Queues.Remove(this);
}

CommandAllocatorState::~CommandAllocatorState() {

}

CommandListState::~CommandListState() {

}

ID3D12GraphicsCommandList *CommandQueueState::PopCommandList() {
    ID3D12GraphicsCommandList* list{nullptr};

    // Free list?
    if (!commandLists.empty()) {
        list = commandLists.back();
        commandLists.pop_back();

        // Reopen
        list->Reset(commandAllocator, nullptr);

        // OK
        return list;
    }

    // Attempt to create
    if (FAILED(parent->object->CreateCommandList(
        0,
        desc.Type,
        commandAllocator,
        nullptr,
        IID_PPV_ARGS(&list)
    ))) {
        return nullptr;
    }

    return list;
}

void CommandQueueState::PushCommandList(ID3D12GraphicsCommandList *list) {
    commandLists.push_back(list);
}
