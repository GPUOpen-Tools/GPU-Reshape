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

#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/CommandAllocatorState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/CommandSignatureState.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/Command/UserCommandBuffer.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Controllers/VersioningController.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backend/IFeature.h>
#include <Backends/DX12/IncrementalFence.h>

static D3D12_COMMAND_LIST_TYPE GetEmulatedCommandListType(D3D12_COMMAND_LIST_TYPE type) {
    switch (type) {
        default:
            return type;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            // Copy queues are emulated on compute, this allows for kernels to be run for validation purposes
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    }
}

void CreateDeviceCommandProxies(DeviceState *state) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(state->features.size()); i++) {
        const ComRef<IFeature>& feature = state->features[i];

        // Get the hook table
        FeatureHookTable hookTable = feature->GetHookTable();

        // Append
        state->featureHookTables.push_back(hookTable);

        /* Create all relevant proxies */

        if (hookTable.dispatch.IsValid()) {
            state->commandListProxies.featureHooks_Dispatch[i] = hookTable.dispatch;
            state->commandListProxies.featureBitSetMask_Dispatch |= (1ull << i);
        }

        if (hookTable.copyResource.IsValid()) {
            state->commandListProxies.featureHooks_CopyBufferRegion[i] = hookTable.copyResource;
            state->commandListProxies.featureHooks_CopyTextureRegion[i] = hookTable.copyResource;
            state->commandListProxies.featureHooks_CopyResource[i] = hookTable.copyResource;
            state->commandListProxies.featureBitSetMask_CopyBufferRegion |= (1ull << i);
            state->commandListProxies.featureBitSetMask_CopyTextureRegion |= (1ull << i);
            state->commandListProxies.featureBitSetMask_CopyResource |= (1ull << i);
        }

        if (hookTable.resolveResource.IsValid()) {
            state->commandListProxies.featureHooks_ResolveSubresource[i] = hookTable.resolveResource;
            state->commandListProxies.featureHooks_ResolveSubresourceRegion[i] = hookTable.resolveResource;
            state->commandListProxies.featureBitSetMask_ResolveSubresource |= (1ull << i);
            state->commandListProxies.featureBitSetMask_ResolveSubresourceRegion |= (1ull << i);
        }

        if (hookTable.clearResource.IsValid()) {
            state->commandListProxies.featureHooks_ClearDepthStencilView[i] = hookTable.clearResource;
            state->commandListProxies.featureHooks_ClearRenderTargetView[i] = hookTable.clearResource;
            state->commandListProxies.featureHooks_ClearUnorderedAccessViewFloat[i] = hookTable.clearResource;
            state->commandListProxies.featureHooks_ClearUnorderedAccessViewUint[i] = hookTable.clearResource;
            state->commandListProxies.featureBitSetMask_ClearDepthStencilView |= (1ull << i);
            state->commandListProxies.featureBitSetMask_ClearRenderTargetView |= (1ull << i);
            state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewFloat |= (1ull << i);
            state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewUint |= (1ull << i);
        }

        if (hookTable.beginRenderPass.IsValid()) {
            state->commandListProxies.featureHooks_BeginRenderPass[i] = hookTable.beginRenderPass;
            state->commandListProxies.featureHooks_OMSetRenderTargets[i] = hookTable.beginRenderPass;
            state->commandListProxies.featureBitSetMask_BeginRenderPass |= (1ull << i);
            state->commandListProxies.featureBitSetMask_OMSetRenderTargets |= (1ull << i);
        }

        if (hookTable.endRenderPass.IsValid()) {
            state->commandListProxies.featureHooks_EndRenderPass[i] = hookTable.endRenderPass;
            state->commandListProxies.featureBitSetMask_EndRenderPass |= (1ull << i);
        }
    }
}

void SetDeviceCommandFeatureSetAndCommit(DeviceState *state, uint64_t featureSet) {
    state->commandListProxies.featureBitSet_Dispatch = state->commandListProxies.featureBitSetMask_Dispatch & featureSet;
    state->commandListProxies.featureBitSet_CopyBufferRegion = state->commandListProxies.featureBitSetMask_CopyBufferRegion & featureSet;
    state->commandListProxies.featureBitSet_CopyTextureRegion = state->commandListProxies.featureBitSetMask_CopyTextureRegion & featureSet;
    state->commandListProxies.featureBitSet_CopyResource = state->commandListProxies.featureBitSetMask_CopyResource & featureSet;
    state->commandListProxies.featureBitSet_ResolveSubresource = state->commandListProxies.featureBitSetMask_ResolveSubresource & featureSet;
    state->commandListProxies.featureBitSet_ClearDepthStencilView = state->commandListProxies.featureBitSetMask_ClearDepthStencilView & featureSet;
    state->commandListProxies.featureBitSet_ClearRenderTargetView = state->commandListProxies.featureBitSetMask_ClearRenderTargetView & featureSet;
    state->commandListProxies.featureBitSet_ClearUnorderedAccessViewUint = state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewUint & featureSet;
    state->commandListProxies.featureBitSet_ClearUnorderedAccessViewFloat = state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewFloat & featureSet;
    state->commandListProxies.featureBitSet_ResolveSubresourceRegion = state->commandListProxies.featureBitSetMask_ResolveSubresourceRegion & featureSet;
    state->commandListProxies.featureBitSet_BeginRenderPass = state->commandListProxies.featureBitSetMask_BeginRenderPass & featureSet;
    state->commandListProxies.featureBitSet_EndRenderPass = state->commandListProxies.featureBitSetMask_EndRenderPass & featureSet;
    state->commandListProxies.featureBitSet_OMSetRenderTargets = state->commandListProxies.featureBitSetMask_OMSetRenderTargets & featureSet;
}

static HRESULT CreateCommandQueueState(ID3D12Device *device, ID3D12CommandQueue* commandQueue, const D3D12_COMMAND_QUEUE_DESC *desc, const IID &riid, void **pCommandQueue) {
    auto table = GetTable(device);

    // Create state
    auto *state = new (table.state->allocators, kAllocStateCommandQueue) CommandQueueState(table.state->allocators.Tag(kAllocStateCommandQueue));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->desc = *desc;
    state->object = commandQueue;

    // Create detours
    commandQueue = CreateDetour(state->allocators, commandQueue, state);

    // Query to external object if requested
    if (pCommandQueue) {
        HRESULT hr = commandQueue->QueryInterface(riid, pCommandQueue);
        if (FAILED(hr)) {
            return hr;
        }

        // Create shared fence
        state->sharedFence = new (table.state->allocators, kAllocStateIncrementalFence) IncrementalFence();
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

HRESULT HookID3D12DeviceCreateCommandQueue(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC *desc, const IID &riid, void **pCommandQueue) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandQueue *commandQueue{nullptr};

    // Get emulated description
    D3D12_COMMAND_QUEUE_DESC emulatedDesc = *desc;
    emulatedDesc.Type = GetEmulatedCommandListType(desc->Type);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandQueue(table.next, &emulatedDesc, __uuidof(ID3D12CommandQueue), reinterpret_cast<void **>(&commandQueue));
    if (FAILED(hr)) {
        return hr;
    }

    // OK
    return CreateCommandQueueState(device, commandQueue, desc, riid, pCommandQueue);
}

HRESULT WINAPI HookID3D12DeviceCreateCommandQueue1(ID3D12Device *device, const D3D12_COMMAND_QUEUE_DESC* desc, const IID& creatorId, const IID& riid, void** pCommandQueue) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandQueue *commandQueue{nullptr};

    // Get emulated description
    D3D12_COMMAND_QUEUE_DESC emulatedDesc = *desc;
    emulatedDesc.Type = GetEmulatedCommandListType(desc->Type);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandQueue1(table.next, &emulatedDesc, creatorId, __uuidof(ID3D12CommandQueue), reinterpret_cast<void **>(&commandQueue));
    if (FAILED(hr)) {
        return hr;
    }

    // OK
    return CreateCommandQueueState(device, commandQueue, desc, riid, pCommandQueue);
}

HRESULT WINAPI HookID3D12DeviceCreateCommandSignature(ID3D12Device *device, const D3D12_COMMAND_SIGNATURE_DESC* pDesc, ID3D12RootSignature* pRootSignature, const IID& riid, void** ppvCommandSignature) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandSignature *commandSignature{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandSignature(table.next, pDesc, Next(pRootSignature), __uuidof(ID3D12CommandSignature), reinterpret_cast<void **>(&commandSignature));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new (table.state->allocators, kAllocStateCommandSignatureState) CommandSignatureState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->object = commandSignature;

    // Filter arguments
    for (uint32_t i = 0; i < pDesc->NumArgumentDescs; i++) {
        switch (pDesc->pArgumentDescs[i].Type) {
            default:
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
                state->activeTypes |= PipelineType::Graphics;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
                state->activeTypes |= PipelineType::Graphics;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                state->activeTypes |= PipelineType::Compute;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS:
                ASSERT(false, "Raytracing is not supported yet");
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
                ASSERT(false, "Mesh shaders are not supported yet");
                break;
        }
    }

    // Create detours
    commandSignature = CreateDetour(state->allocators, commandSignature, state);

    // Query to external object if requested
    if (ppvCommandSignature) {
        HRESULT hr = commandSignature->QueryInterface(riid, ppvCommandSignature);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    commandSignature->Release();

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
    HRESULT hr = table.bottom->next_CreateCommandAllocator(table.next, GetEmulatedCommandListType(type), __uuidof(ID3D12CommandAllocator), reinterpret_cast<void **>(&commandAllocator));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new (table.state->allocators, kAllocStateCommandAllocator) CommandAllocatorState();
    state->allocators = table.state->allocators;
    state->userType = type;
    state->parent = device;

    // Create detours
    commandAllocator = CreateDetour(state->allocators, commandAllocator, state);

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

static void BeginCommandList(DeviceState* device, CommandListState* state, ID3D12PipelineState* initialState, bool isHotSwap) {
    // Inform the streamer
    device->exportStreamer->BeginCommandList(state->streamState, state->object);

    // Inform the streamer of a new pipeline
    if (initialState) {
        device->exportStreamer->BindPipeline(state->streamState, GetState(initialState), initialState, isHotSwap, state->object);
    }

    // Pass down the controller
    device->instrumentationController->BeginCommandList();

    // Copy proxy table
    state->proxies = device->commandListProxies;
    state->proxies.context = &state->userContext;

    // Cleanup user context
    state->userContext.eventStack.Flush();
    state->userContext.eventStack.SetRemapping(device->eventRemappingTable);
    state->userContext.handle = reinterpret_cast<CommandContextHandle>(state);

    // Set stream context handle
    state->streamState->commandContextHandle = state->userContext.handle;

    // Invoke proxies
    for (const FeatureHookTable &table: device->featureHookTables) {
        table.open.TryInvoke(&state->userContext);
    }
}

HRESULT CreateCommandListState(ID3D12Device *device, ID3D12CommandList* commandList, D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState *initialState, ID3D12PipelineState* hotSwap, const IID &riid, void **pCommandList) {
    auto table = GetTable(device);

    // Create state
    auto *state = new (table.state->allocators, kAllocStateCommandList) CommandListState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->userType = type;
    state->object = static_cast<ID3D12GraphicsCommandList*>(commandList);

    // Create detours
    commandList = CreateDetour(state->allocators, commandList, state);

    // Query to external object if requested
    if (pCommandList) {
        HRESULT hr = commandList->QueryInterface(riid, pCommandList);
        if (FAILED(hr)) {
            return hr;
        }

        // Create export state
        state->streamState = table.state->exportStreamer->AllocateStreamState();

        // Handle sub-systems
        BeginCommandList(table.state, state, initialState, hotSwap != nullptr);
    }

    // Cleanup
    commandList->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateCommandList(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, ID3D12PipelineState *initialState, const IID &riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(initialState);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, GetEmulatedCommandListType(type), Next(allocator), hotSwap ? hotSwap : Next(initialState), __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    return CreateCommandListState(device, commandList, type, initialState, hotSwap, riid, pCommandList);
}

HRESULT WINAPI HookID3D12DeviceCreateCommandList1(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_LIST_FLAGS flags, const IID &riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList1(table.next, nodeMask, GetEmulatedCommandListType(type), flags, __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    return CreateCommandListState(device, commandList, type, nullptr, nullptr, riid, pCommandList);
}

HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList *list, ID3D12CommandAllocator *allocator, ID3D12PipelineState *state) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Pass down the controller
    device.state->instrumentationController->BeginCommandList();

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(state);

    // Pass down callchain
    HRESULT result = table.bottom->next_Reset(table.next, Next(allocator), hotSwap ? hotSwap : Next(state));
    if (FAILED(result)) {
        return result;
    }

    // Handle sub-systems
    BeginCommandList(device.state, table.state, state, hotSwap != nullptr);

    // OK
    return S_OK;
}

void WINAPI HookID3D12CommandListSetDescriptorHeaps(ID3D12CommandList* list, UINT NumDescriptorHeaps, ID3D12DescriptorHeap *const *ppDescriptorHeaps) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Allocate unwrapped
    auto *unwrapped = ALLOCA_ARRAY(ID3D12DescriptorHeap*, NumDescriptorHeaps);
    for (uint32_t i = 0; i < NumDescriptorHeaps; i++) {
        auto heapTable = GetTable(ppDescriptorHeaps[i]);
        unwrapped[i] = heapTable.next;
    }

    // Pass down callchain
    table.bottom->next_SetDescriptorHeaps(table.next, NumDescriptorHeaps, unwrapped);

    // Let the streamer handle allocations
    for (uint32_t i = 0; i < NumDescriptorHeaps; i++) {
        auto heapTable = GetTable(ppDescriptorHeaps[i]);
        device.state->exportStreamer->SetDescriptorHeap(table.state->streamState, heapTable.state, table.state->object);
    }
}

void WINAPI HookID3D12CommandListSetComputeRootDescriptorTable(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootDescriptorTable(table.state->streamState, RootParameterIndex, BaseDescriptor);

    // Pass down call chain
    table.next->SetComputeRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

void WINAPI HookID3D12CommandListSetGraphicsRootDescriptorTable(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootDescriptorTable(table.state->streamState, RootParameterIndex, BaseDescriptor);

    // Pass down call chain
    table.next->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

void WINAPI HookID3D12CommandListSetComputeRootShaderResourceView(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootShaderResourceView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetComputeRootShaderResourceView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListSetGraphicsRootShaderResourceView(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootShaderResourceView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetGraphicsRootShaderResourceView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListSetComputeRootUnorderedAccessView(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootUnorderedAccessView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetComputeRootUnorderedAccessView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListSetGraphicsRootUnorderedAccessView(ID3D12CommandList* list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootUnorderedAccessView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetGraphicsRootUnorderedAccessView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListSetComputeRootConstantBufferView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootConstantBufferView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetComputeRootConstantBufferView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListSetGraphicsRootConstantBufferView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootConstantBufferView(table.state->streamState, RootParameterIndex, BufferLocation);

    // Pass down call chain
    table.next->SetGraphicsRootConstantBufferView(RootParameterIndex, BufferLocation);
}

void WINAPI HookID3D12CommandListCopyBufferRegion(ID3D12CommandList *list, ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) {
    auto table = GetTable(list);

    // Pass down callchain
    table.next->CopyBufferRegion(Next(pDstBuffer), DstOffset, Next(pSrcBuffer), SrcOffset, NumBytes);
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

static void CommitGraphics(DeviceState* device, CommandListState* list) {
    // Commit all commands prior to binding
    CommitCommands(list);

    // Inform the streamer
    device->exportStreamer->CommitGraphics(list->streamState, list->object);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = list->userContext.eventStack.GetGraphicsDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            list->object->SetGraphicsRoot32BitConstant(
                list->streamState->pipeline->signature->userRootCount + 2u,
                list->userContext.eventStack.GetData()[index],
                index
            );

            // Next!
            bitMask &= ~(1ull << index);
        }

        // Cleanup
        list->userContext.eventStack.FlushGraphics();
    }
}

static void CommitCompute(DeviceState* device, CommandListState* list) {
    // Commit all commands prior to binding
    CommitCommands(list);

    // Inform the streamer
    device->exportStreamer->CommitCompute(list->streamState, list->object);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = list->userContext.eventStack.GetComputeDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            list->object->SetComputeRoot32BitConstant(
                list->streamState->pipeline->signature->userRootCount + 2u,
                list->userContext.eventStack.GetData()[index],
                index
            );

            // Next!
            bitMask &= ~(1ull << index);
        }

        // Cleanup
        list->userContext.eventStack.FlushCompute();
    }
}

void WINAPI HookID3D12CommandListDrawInstanced(ID3D12CommandList* list, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Commit all pending graphics
    CommitGraphics(device.state, table.state);

    // Pass down callchain
    table.next->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void WINAPI HookID3D12CommandListDrawIndexedInstanced(ID3D12CommandList* list, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Commit all pending graphics
    CommitGraphics(device.state, table.state);

    // Pass down callchain
    table.next->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void WINAPI HookID3D12CommandListDispatch(ID3D12CommandList* list, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Commit all pending compute
    CommitCompute(device.state, table.state);

    // Pass down callchain
    table.next->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void WINAPI HookID3D12CommandListSetComputeRoot32BitConstant(ID3D12CommandList *list, UINT RootParameterIndex, UINT SrcData, UINT DestOffsetIn32BitValues) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootConstants(table.state->streamState, RootParameterIndex, &SrcData, sizeof(UINT), DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetComputeRoot32BitConstant(RootParameterIndex, SrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListSetGraphicsRoot32BitConstant(ID3D12CommandList *list, UINT RootParameterIndex, UINT SrcData, UINT DestOffsetIn32BitValues) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootConstants(table.state->streamState, RootParameterIndex, &SrcData, sizeof(UINT), DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetGraphicsRoot32BitConstant(RootParameterIndex, SrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListSetComputeRoot32BitConstants(ID3D12CommandList *list, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void *pSrcData, UINT DestOffsetIn32BitValues) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetComputeRootConstants(table.state->streamState, RootParameterIndex, &pSrcData, sizeof(UINT) * Num32BitValuesToSet, DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListSetGraphicsRoot32BitConstants(ID3D12CommandList *list, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void *pSrcData, UINT DestOffsetIn32BitValues) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootConstants(table.state->streamState, RootParameterIndex, &pSrcData, sizeof(UINT) * Num32BitValuesToSet, DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListExecuteIndirect(ID3D12CommandList* list, ID3D12CommandSignature *pCommandSignature, UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset, ID3D12Resource *pCountBuffer, UINT64 CountBufferOffset) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get signature
    auto signatureTable = GetTable(pCommandSignature);

    // Commit compute if needed
    if (signatureTable.state->activeTypes & PipelineType::Compute) {
        device.state->exportStreamer->CommitCompute(table.state->streamState, table.state->object);
    }
    
    // Commit graphics if needed
    if (signatureTable.state->activeTypes & PipelineType::Graphics) {
        device.state->exportStreamer->CommitGraphics(table.state->streamState, table.state->object);
    }

    // Pass down callchain
    table.next->ExecuteIndirect(
        signatureTable.next, 
        MaxCommandCount, 
        Next(pArgumentBuffer), 
        ArgumentBufferOffset, 
        Next(pCountBuffer), 
        CountBufferOffset
    );
}

void WINAPI HookID3D12CommandListSetGraphicsRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature) {
    auto table = GetTable(list);
    auto rsTable = GetTable(rootSignature);

    // Get device
    auto device = GetTable(table.state->parent);

    // Pass down callchain
    table.bottom->next_SetGraphicsRootSignature(table.next, rsTable.next);

    // Inform the streamer of a new root signature
    device.state->exportStreamer->SetGraphicsRootSignature(table.state->streamState, rsTable.state, table.state->object);
}

void WINAPI HookID3D12CommandListSetComputeRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature) {
    auto table = GetTable(list);
    auto rsTable = GetTable(rootSignature);

    // Get device
    auto device = GetTable(table.state->parent);

    // Pass down callchain
    table.bottom->next_SetComputeRootSignature(table.next, rsTable.next);

    // Inform the streamer of a new root signature
    device.state->exportStreamer->SetComputeRootSignature(table.state->streamState, rsTable.state, table.state->object);
}

HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList *list) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->CloseCommandList(table.state->streamState);

    // Invoke proxies
    for (const FeatureHookTable &proxyTable: device.state->featureHookTables) {
        proxyTable.close.TryInvoke(table.state->userContext.handle);
    }

    // Pass down callchain
    return table.bottom->next_Close(table.next);
}

void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList *list, ID3D12PipelineState *pipeline) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get hot swap
    ID3D12PipelineState *hotSwap = GetHotSwapPipeline(pipeline);

    // Pass down callchain
    table.bottom->next_SetPipelineState(table.next, hotSwap ? hotSwap : Next(pipeline));

    // Inform the streamer of a new pipeline
    device.state->exportStreamer->BindPipeline(table.state->streamState, GetState(pipeline), hotSwap, hotSwap != nullptr, table.state->object);
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

    // Get device
    auto device = GetTable(table.state->parent);

    // Process any remaining work on the queue
    device.state->exportStreamer->Process(table.state);

    // Special case, invoke a device sync point during empty submissions
    if (count == 0) {
        BridgeDeviceSyncPoint(device.state);
    }

    // Allocate submission segment
    ShaderExportStreamSegment* segment = device.state->exportStreamer->AllocateSegment();
    
    // Inform the controller of the segmentation point
    segment->versionSegPoint = device.state->versioningController->BranchOnSegmentationPoint();

    // Final list of commands to submit
    TrivialStackVector<ID3D12CommandList*, 64u> unwrapped;
    
    // Reserve a slot for the streaming pre patching
    unwrapped.Add(nullptr);

    // Process all lists
    for (uint32_t i = 0; i < count; i++) {
        auto listTable = GetTable(lists[i]);

        // Create streamer allocation association
        device.state->exportStreamer->MapSegment(listTable.state->streamState, segment);

        // Pass down unwrapped
        unwrapped.Add(listTable.next);
    }

    // Record the streaming pre patching
    unwrapped[0] = device.state->exportStreamer->RecordPreCommandList(table.state, segment);

    // Record the streaming post patching
    unwrapped.Add(device.state->exportStreamer->RecordPostCommandList(table.state, segment));

    // Pass down callchain
    table.bottom->next_ExecuteCommandLists(table.next, static_cast<uint32_t>(unwrapped.Size()), unwrapped.Data());

    // Process all again for proxies
    for (uint32_t i = 0; i < count; i++) {
        auto listTable = GetTable(lists[i]);

        // Invoke all proxies
        for (const FeatureHookTable &proxyTable: device.state->featureHookTables) {
            proxyTable.submit.TryInvoke(listTable.state->userContext.handle);
        }
    }

    // Notify streamer of submission
    device.state->exportStreamer->Enqueue(table.state, segment);
}

void WINAPI HookID3D12CommandQueueGetDesc(ID3D12CommandQueue *_this, D3D12_COMMAND_QUEUE_DESC* out) {
    auto table = GetTable(_this);

    // Report internal user description, not underlying
    *out = table.state->desc;
}

HRESULT HookID3D12CommandQueueGetDevice(ID3D12CommandQueue *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

HRESULT HookID3D12CommandListGetDevice(ID3D12CommandList *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

D3D12_COMMAND_LIST_TYPE WINAPI HookID3D12CommandListGetType(ID3D12CommandList *_this) {
    auto table = GetTable(_this);

    // Report internal user type
    return table.state->userType;
}

HRESULT HookID3D12CommandAllocatorGetDevice(ID3D12CommandAllocator *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

CommandQueueState::~CommandQueueState() {
    // Get device
    auto device = GetTable(parent);

    // Clean the streaming state for this queue
    device.state->exportStreamer->Process(this);

    // Remove state
    device.state->states_Queues.Remove(this);
}

CommandAllocatorState::~CommandAllocatorState() {

}

CommandListState::~CommandListState() {

}

ImmediateCommandList CommandQueueState::PopCommandList() {
    ImmediateCommandList list;

    // Get device
    auto device = GetTable(parent);

    // Free list?
    if (!commandLists.empty()) {
        list = commandLists.back();
        commandLists.pop_back();

        // Reopen
        HRESULT hr = list.commandList->Reset(list.allocator, nullptr);
        ASSERT(SUCCEEDED(hr), "Failed to reset command list");

        // OK
        return list;
    }

    // Use the queues emulated type
    D3D12_COMMAND_LIST_TYPE emulatedType = GetEmulatedCommandListType(desc.Type);

    // Create allocator
    if (FAILED(device.state->object->CreateCommandAllocator(emulatedType, IID_PPV_ARGS(&list.allocator)))) {
        return {};
    }

    // Attempt to create
    if (FAILED(device.state->object->CreateCommandList(
        0,
        emulatedType,
        list.allocator,
        nullptr,
        IID_PPV_ARGS(&list.commandList)
    ))) {
        return {};
    }

    // OK
    return list;
}

void CommandQueueState::PushCommandList(const ImmediateCommandList& list) {
    HRESULT hr = list.allocator->Reset();
    ASSERT(SUCCEEDED(hr), "Pushing in-flight immediate command list");

    // Append
    commandLists.push_back(list);
}
