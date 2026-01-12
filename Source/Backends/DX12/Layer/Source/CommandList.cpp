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

#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/RenderPass.h>
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
#include <Backends/DX12/QueueSegmentAllocator.h>
#include <Backends/DX12/IncrementalFence.h>
#include <Backends/DX12/Scheduler/Scheduler.h>
#include <Backends/DX12/Export/ShaderExportStreamStateBarrierTracking.h>
#include <Backends/DX12/Export/ShaderExportStreamStateRaytracingCache.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>

// Shared
#include <Shared/ShaderBackendMessage.h>

// Backend
#include <Backend/IL/DeviceCommand.h>
#include <Backend/SubmissionContext.h>
#include <Backend/IL/Execution/ExecutionInfo.h>
#include <Backend/IFeature.h>

// WinPixEventRuntime
#include <WinPixEventRuntime/pix3.h>

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
            state->commandListProxies.featureHooks_CopyTiles[i] = hookTable.copyResource;
            state->commandListProxies.featureBitSetMask_CopyBufferRegion |= (1ull << i);
            state->commandListProxies.featureBitSetMask_CopyTextureRegion |= (1ull << i);
            state->commandListProxies.featureBitSetMask_CopyResource |= (1ull << i);
            state->commandListProxies.featureBitSetMask_CopyTiles |= (1ull << i);
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

        if (hookTable.discardResource.IsValid()) {
            state->commandListProxies.featureHooks_DiscardResource[i] = hookTable.discardResource;
            state->commandListProxies.featureBitSetMask_DiscardResource |= (1ull << i);
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

        if (hookTable.deviceCommand.IsValid()) {
            state->commandListProxies.featureHooks_ExecuteIndirect[i] = hookTable.deviceCommand;
            state->commandListProxies.featureBitSetMask_ExecuteIndirect |= (1ull << i);
        }
    }
}

void SetDeviceCommandFeatureSetAndCommit(DeviceState *state, uint64_t featureSet) {
    state->commandListProxies.featureBitSet_Dispatch = state->commandListProxies.featureBitSetMask_Dispatch & featureSet;
    state->commandListProxies.featureBitSet_CopyBufferRegion = state->commandListProxies.featureBitSetMask_CopyBufferRegion & featureSet;
    state->commandListProxies.featureBitSet_CopyTextureRegion = state->commandListProxies.featureBitSetMask_CopyTextureRegion & featureSet;
    state->commandListProxies.featureBitSet_CopyResource = state->commandListProxies.featureBitSetMask_CopyResource & featureSet;
    state->commandListProxies.featureBitSet_CopyTiles = state->commandListProxies.featureBitSetMask_CopyTiles & featureSet;
    state->commandListProxies.featureBitSet_ResolveSubresource = state->commandListProxies.featureBitSetMask_ResolveSubresource & featureSet;
    state->commandListProxies.featureBitSet_ClearDepthStencilView = state->commandListProxies.featureBitSetMask_ClearDepthStencilView & featureSet;
    state->commandListProxies.featureBitSet_ClearRenderTargetView = state->commandListProxies.featureBitSetMask_ClearRenderTargetView & featureSet;
    state->commandListProxies.featureBitSet_ClearUnorderedAccessViewUint = state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewUint & featureSet;
    state->commandListProxies.featureBitSet_ClearUnorderedAccessViewFloat = state->commandListProxies.featureBitSetMask_ClearUnorderedAccessViewFloat & featureSet;
    state->commandListProxies.featureBitSet_ResolveSubresourceRegion = state->commandListProxies.featureBitSetMask_ResolveSubresourceRegion & featureSet;
    state->commandListProxies.featureBitSet_DiscardResource = state->commandListProxies.featureBitSetMask_DiscardResource & featureSet;
    state->commandListProxies.featureBitSet_BeginRenderPass = state->commandListProxies.featureBitSetMask_BeginRenderPass & featureSet;
    state->commandListProxies.featureBitSet_EndRenderPass = state->commandListProxies.featureBitSetMask_EndRenderPass & featureSet;
    state->commandListProxies.featureBitSet_OMSetRenderTargets = state->commandListProxies.featureBitSetMask_OMSetRenderTargets & featureSet;
    state->commandListProxies.featureBitSet_ExecuteIndirect = state->commandListProxies.featureBitSetMask_ExecuteIndirect & featureSet;
}

static HRESULT CreateCommandQueueState(ID3D12Device *device, ID3D12CommandQueue* commandQueue, const D3D12_COMMAND_QUEUE_DESC *desc, const IID &riid, void **pCommandQueue) {
    auto table = GetTable(device);

    // Create state
    auto *state = new (table.state->allocators, kAllocStateCommandQueue) CommandQueueState(table.state->allocators.Tag(kAllocStateCommandQueue));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->desc = *desc;
    state->object = commandQueue;
    
    // Setup shared context
    state->executor.context.eventStack.SetRemapping(table.state->eventRemappingTable);
    state->executor.context.handle = reinterpret_cast<CommandContextHandle>(commandQueue);

    // Keep reference
    device->AddRef();

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

static void CreateCommandSignatureCommandData(ID3D12Device *device, const D3D12_COMMAND_SIGNATURE_DESC* pDesc, CommandSignatureState* state) {
    std::vector<uint32_t> dwords;

    // Append header
    dwords.push_back(pDesc->NumArgumentDescs);
    dwords.push_back(pDesc->ByteStride);

    // Append command types
    for (uint32_t i = 0; i < pDesc->NumArgumentDescs; i++) {
        switch (pDesc->pArgumentDescs[i].Type) {
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
                dwords.push_back(static_cast<uint32_t>(IL::DeviceCommandType::Dispatch));
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
            case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS:
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
            case D3D12_INDIRECT_ARGUMENT_TYPE_INCREMENTING_CONSTANT:
                dwords.push_back(static_cast<uint32_t>(IL::DeviceCommandType::Unexposed));
                break;
        }
    }

    // Resource info
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = sizeof(uint32_t) * dwords.size();
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    // Standard upload
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Create the resource
    device->CreateCommittedResource(
        &heapProperties, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
        __uuidof(ID3D12Resource), reinterpret_cast<void**>(&state->deviceCommandAllocation)
    );

    // Map the range
    void* data{nullptr};
    state->deviceCommandAllocation->Map(0, nullptr, &data);

    // Copy over the data
    std::memcpy(data, dwords.data(), desc.Width);

    // Unmap
    state->deviceCommandAllocation->Unmap(0, nullptr);
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
    state->byteStride = pDesc->ByteStride;
    state->arguments.reserve(pDesc->NumArgumentDescs);

    // Filter arguments
    for (uint32_t i = 0; i < pDesc->NumArgumentDescs; i++) {
        state->arguments.push_back(pDesc->pArgumentDescs[i]);
        
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
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH:
                state->activeTypes = PipelineType::Graphics;
                break;
            case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS:
                state->activeTypes = PipelineType::StateObject;
                break;
        }
    }

    // Create the signature data for instrumentation
    CreateCommandSignatureCommandData(device, pDesc, state);

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

static void FlushCommandQueueContext(DeviceState* device, CommandQueueState* queue) {
    std::lock_guard guard(queue->executor.mutex);
    
    if (!queue->executor.context.buffer.Count()) {
        return;
    }
    
    // Execute context immediately
    device->queueSegmentAllocator->ExecuteImmediate(queue, queue->executor.context);
    
    // Cleanup
    queue->executor.context.buffer.Clear();
    queue->executor.context.eventStack.Flush();
}

HRESULT WINAPI HookID3D12CommandSignatureGetDevice(ID3D12CommandSignature* _this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue* queue, ID3D12Fence* pFence, UINT64 Value) {
    auto table = GetTable(queue);

    // Flush any pending work
    FlushCommandQueueContext(GetState(table.state->parent), table.state);

    // Pass down callchain
    return table.bottom->next_Signal(table.next, UnwrapObject(pFence), Value);
}

HRESULT WINAPI HookID3D12CommandQueueWait(ID3D12CommandQueue* queue, ID3D12Fence* pFence, UINT64 Value) {
    auto table = GetTable(queue);

    // Flush any pending work
    FlushCommandQueueContext(GetState(table.state->parent), table.state);

    // Pass down callchain
    return table.bottom->next_Wait(table.next, UnwrapObject(pFence), Value);
}

void WINAPI HookID3D12CommandQueueCopyTileMappings(ID3D12CommandQueue* queue, ID3D12Resource* pDstResource, const D3D12_TILED_RESOURCE_COORDINATE* pDstRegionStartCoordinate, ID3D12Resource* pSrcResource, const D3D12_TILED_RESOURCE_COORDINATE* pSrcRegionStartCoordinate, const D3D12_TILE_REGION_SIZE* pRegionSize, D3D12_TILE_MAPPING_FLAGS Flags) {
    auto table = GetTable(queue);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get resources
    auto srcTable = GetTable(pSrcResource);
    auto dstTable = GetTable(pDstResource);

    // Pass down callchain
    table.bottom->next_CopyTileMappings(table.next, dstTable.next, pDstRegionStartCoordinate, srcTable.next, pSrcRegionStartCoordinate, pRegionSize, Flags);

    // Create infos
    ResourceInfo srcInfo = GetResourceInfoFor(srcTable.state);
    ResourceInfo dstInfo = GetResourceInfoFor(dstTable.state);

    // Invoke proxies
    std::lock_guard guard(table.state->executor.mutex);
    for (const FeatureHookTable &feature: device.state->featureHookTables) {
        feature.copyResource.TryInvoke(&table.state->executor.context, srcInfo, dstInfo);
    }
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

HRESULT HookID3D12CommandAllocatorReset(ID3D12CommandAllocator *_this) {
    auto table = GetTable(_this);

    // Get device
    auto device = GetTable(table.state->parent);

    // Serialize
    {
        std::lock_guard guard(table.state->lock);
        
        // The allocator owns the lifetimes of the streaming states
        // If an allocator has been reset, all the submissions are done, so free the states up
        for (ShaderExportStreamState *streamState : table.state->streamStates) {
            device.state->exportStreamer->Free(streamState);
        }
    
        // Cleanup
        table.state->streamStates.clear();
    }

    // Pass to allocator
    return table.next->Reset();
}

static PipelineInstrument *GetHotSwapPipeline(ID3D12PipelineState *initialState) {
    if (!initialState) {
        return nullptr;
    }
    
    // PipelineState0 only supports graphics/compute
    PipelineState* state = GetState(initialState);
    if (state->type != PipelineType::Graphics && state->type != PipelineType::Compute) {
        return nullptr;
    }

    // Available hot swap?
    return state->hotSwapObject.load();
}

static void BeginCommandList(DeviceState* device, CommandListState* state, ID3D12CommandAllocator* allocator, PipelineState* initialState, ID3D12PipelineState* pipelineObject, PipelineInstrument* instrument) {
    auto allocatorTable = GetTable(allocator);

    // Either the state must be zero, or an allocator already owns it
    ASSERT(!state->streamState || state->owningAllocator, "Leaking streaming state");
    
    // If the allocator has changed
    if (state->owningAllocator != allocator) {
        // Has previous instance?
        if (state->allocatorSlotId != kInvalidSlotId) {
            auto owningTable = GetTable(state->owningAllocator);

            // Remove from owning
            std::lock_guard guard(owningTable.state->lock);
            owningTable.state->commandLists.Remove(state);
        }

        // New owner
        state->owningAllocator = allocator;
        
        // Add to allocator tracking
        std::lock_guard guard(allocatorTable.state->lock);
        allocatorTable.state->commandLists.Add(state);
    }

    // Create export state
    // Ideally re-used behind the scenes
    state->streamState = device->exportStreamer->AllocateStreamState();

    // Add to allocator, effectively owns this streaming state
    {
        std::lock_guard guard(allocatorTable.state->lock);
        allocatorTable.state->streamStates.push_back(state->streamState);
    }
    
    // Inform the streamer
    device->exportStreamer->BeginCommandList(state->streamState, state->object);

    // Inform the streamer of a new pipeline
    if (initialState) {
        device->exportStreamer->BindPipeline(state->streamState, initialState, pipelineObject, instrument, state->object);
    }

    // Copy proxy table
    state->proxies = device->commandListProxies;
    state->proxies.context = &state->userContext;

    // Cleanup user context
    state->userContext.buffer.Clear();
    state->userContext.eventStack.Flush();
    state->userContext.eventStack.SetRemapping(device->eventRemappingTable);
    state->userContext.handle = reinterpret_cast<CommandContextHandle>(state->streamState);

    // Set stream context handle
    state->streamState->commandContextHandle = state->userContext.handle;

    // Invoke proxies
    for (const FeatureHookTable &table: device->featureHookTables) {
        table.open.TryInvoke(&state->userContext);
    }
}

HRESULT CreateCommandListState(ID3D12Device *device, ID3D12CommandList* commandList, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *allocator, PipelineState *initialState, ID3D12PipelineState* pipelineObject, PipelineInstrument* instrument, bool opened, const IID &riid, void **pCommandList) {
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

        // Handle sub-systems
        if (opened) {
            BeginCommandList(table.state, state, allocator, initialState, pipelineObject, instrument);
        }
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

    // Pass down the controller
    table.state->instrumentationController->ConditionalWaitForCompletion();

    // Get hot swap
    PipelineInstrument *instrument = GetHotSwapPipeline(initialState);

    // Active pipeline
    ID3D12PipelineState* pipelineObject = instrument ? static_cast<ID3D12PipelineState*>(instrument->object) : Next(initialState);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList(table.next, nodeMask, GetEmulatedCommandListType(type), Next(allocator), pipelineObject, __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    return CreateCommandListState(device, commandList, type, allocator, GetState(initialState), pipelineObject, instrument, true, riid, pCommandList);
}

HRESULT WINAPI HookID3D12DeviceCreateCommandList1(ID3D12Device *device, UINT nodeMask, D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_LIST_FLAGS flags, const IID &riid, void **pCommandList) {
    auto table = GetTable(device);

    // Object
    ID3D12CommandList *commandList{nullptr};

    // Pass down the controller
    table.state->instrumentationController->ConditionalWaitForCompletion();

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommandList1(table.next, nodeMask, GetEmulatedCommandListType(type), flags, __uuidof(ID3D12CommandList), reinterpret_cast<void **>(&commandList));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    return CreateCommandListState(device, commandList, type, nullptr, nullptr, nullptr, nullptr, false, riid, pCommandList);
}

HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList *list, ID3D12CommandAllocator *allocator, ID3D12PipelineState *state) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Pass down the controller
    device.state->instrumentationController->ConditionalWaitForCompletion();

    // Get hot swap
    PipelineInstrument *instrument = GetHotSwapPipeline(state);

    // Assigned object
    ID3D12PipelineState* pipelineObject = instrument ? static_cast<ID3D12PipelineState*>(instrument->object) : Next(state);

    // Pass down callchain
    HRESULT result = table.bottom->next_Reset(table.next, Next(allocator), pipelineObject);
    if (FAILED(result)) {
        return result;
    }

    // Handle sub-systems
    BeginCommandList(device.state, table.state, allocator, GetState(state), pipelineObject, instrument);

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

ID3D12Resource* GetStreamingStateBackendMessages(CommandListState *state) {
    auto device = GetTable(state->parent);

    // Target messages
    ShaderExportStreamStateBackendMessages& messages = state->streamState->backendMessages;

    // Create if needed
    if (!messages.allocation.resource) {
        // Allocate buffer
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        desc.Width = BackendMessageBufferSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleDesc.Count = 1;
        messages.allocation = device.state->deviceAllocator->Allocate(desc, AllocationResidency::HostVisible);
    }

    // Lazy clear
    if (messages.pendingInitialization) {
        uint32_t data = 0;
        state->streamState->constantAllocator.StageData(
            device.state->deviceAllocator, state->object,
            messages.allocation.resource, 0,
            &data, sizeof(data)
        );
    }

    // OK
    return messages.allocation.resource;
}

ShaderExportStreamStateBarrierTracking* GetBarrierTracking(CommandListState* state) {
    // Allocate on demand
    if (!state->streamState->barrierTracking) {
        state->streamState->barrierTracking = new (state->allocators) ShaderExportStreamStateBarrierTracking();
    }

    // OK
    return state->streamState->barrierTracking;
}

ShaderExportStreamStateRaytracingCache* GetRaytracingCache(CommandListState* state) {
    // Allocate on demand
    if (!state->streamState->raytracingCache) {
        state->streamState->raytracingCache = new (state->allocators) ShaderExportStreamStateRaytracingCache();
    }

    // OK
    return state->streamState->raytracingCache;
}

#ifndef NDEBUG
void AddDebugStream(CommandListState *state, ID3D12Resource *resource, uint64_t offset, uint64_t length, D3D12_RESOURCE_STATES layout, const std::string &name) {
    auto device = GetTable(state->parent);

    // Transition to copy source
    if (layout != D3D12_RESOURCE_STATE_COPY_SOURCE) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = layout;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        state->object->ResourceBarrier(1u, &barrier);
    }

    // Copy over device to host data
    ShaderExportConstantAllocation hostAllocation = state->streamState->constantAllocator.Allocate(device.state->deviceAllocator, length);
    state->object->CopyBufferRegion(
        hostAllocation.resource, hostAllocation.offset,
        resource, 0,
        length
    );

    // Add to pending streaming
    state->streamState->debugStreams.push_back(ShaderExportStreamStateDebugStream {
        .name = name,
        .resource = hostAllocation.resource,
        .offset = hostAllocation.offset,
        .length = length
    });

    // Transition to original
    if (layout != D3D12_RESOURCE_STATE_COPY_SOURCE) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = layout;
        state->object->ResourceBarrier(1u, &barrier);
    }
}

void AddDebugStream(CommandListState* state, const ShaderExportDeviceAllocation& allocation, D3D12_RESOURCE_STATES layout, const std::string& name) {
    AddDebugStream(state, allocation.allocation.resource, 0, allocation.length, layout, name);
}
#endif // NDEBUG

static void HandleBarrierCacheInvalidation(CommandListState* state, ResourceState* resource) {
    // No tracking? No invalidation
    if (!state->streamState->barrierTracking) {
        return;
    }

    // Find entry
    auto it = state->streamState->barrierTracking->resources.find(resource);
    if (it == state->streamState->barrierTracking->resources.end()) {
        return;
    }

    // Invalidate raytracing state
    if (it->second & ShaderExportStreamStateBarrierFlag::Raytracing) {
        state->streamState->raytracingCache->Invalidate(resource);
    }

    // No longer tracked
    state->streamState->barrierTracking->resources.erase(it);
}

static void HandleGlobalBarrierCacheInvalidation(CommandListState* state) {
    // Remove all tracking
    if (ShaderExportStreamStateBarrierTracking *tracking = state->streamState->barrierTracking) {
        tracking->Clear();
    }

    // Clean all raytracing caches
    if (ShaderExportStreamStateRaytracingCache *cache = state->streamState->raytracingCache) {
        cache->Clear();
    }
}

void WINAPI HookID3D12CommandListResourceBarrier(ID3D12CommandList* list, UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers) {
    auto table = GetTable(list);

    // Copy wrapper barriers
    TrivialStackVector<D3D12_RESOURCE_BARRIER, 16> barriers;

    // Unwrap all objects
    for (uint32_t i = 0; i < NumBarriers; i++) {
        switch (pBarriers[i].Type) {
            case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION: {
                D3D12_RESOURCE_BARRIER barrier = pBarriers[i];
                
                // Invalidate dependent caching
                auto resourceTable = GetTable(barrier.Transition.pResource);
                HandleBarrierCacheInvalidation(table.state, resourceTable.state);
                
                barrier.Transition.pResource = resourceTable.next;
                barriers.Add(barrier);
                break;
            }
            case D3D12_RESOURCE_BARRIER_TYPE_ALIASING: {
                D3D12_RESOURCE_BARRIER barrier = pBarriers[i];
                
                if (barrier.Aliasing.pResourceBefore) {
                    auto resourceTable = GetTable(barrier.Aliasing.pResourceBefore);

                    // Invalidate dependent caching
                    HandleBarrierCacheInvalidation(table.state, resourceTable.state);

                    // If emulated, this barrier is irrelevant
                    if (resourceTable.state->isEmulatedComitted) {
                        continue;
                    }
                    
                    barrier.Aliasing.pResourceBefore = resourceTable.next;
                }
                
                if (barrier.Aliasing.pResourceAfter) {
                    auto resourceTable = GetTable(barrier.Aliasing.pResourceAfter);

                    // Invalidate dependent caching
                    HandleBarrierCacheInvalidation(table.state, resourceTable.state);
                    
                    // If emulated, this barrier is irrelevant
                    if (resourceTable.state->isEmulatedComitted) {
                        continue;
                    }
                    
                    barrier.Aliasing.pResourceAfter = resourceTable.next;
                }
                
                barriers.Add(barrier);
                break;
            }
            case D3D12_RESOURCE_BARRIER_TYPE_UAV: {
                D3D12_RESOURCE_BARRIER barrier = pBarriers[i];
                
                if (barrier.UAV.pResource) {
                    auto resourceTable = GetTable(barrier.Transition.pResource);
                    barrier.UAV.pResource = resourceTable.next;

                    // Invalidate dependent caching
                    HandleBarrierCacheInvalidation(table.state, resourceTable.state);
                } else {
                    // Since we don't know what resource was referenced, just invalidate everything
                    HandleGlobalBarrierCacheInvalidation(table.state);
                }
                
                barriers.Add(barrier);
                break;
            }
        }
    }

    // Aliasing may ignore some barriers
    if (!barriers.Size()) {
        return;
    }

    // Pass down callchain
    table.bottom->next_ResourceBarrier(table.next, static_cast<uint32_t>(barriers.Size()), barriers.Data());
}

void WINAPI HookID3D12CommandListBarrier(ID3D12CommandList *list, UINT NumBarrierGroups, const D3D12_BARRIER_GROUP *pBarrierGroups) {
    auto table = GetTable(list);

    // Current offsets
    uint32_t globalBarrierOffset{0};
    uint32_t textureBarrierOffset{0};
    uint32_t bufferBarrierOffset{0};

    // Count all barrier types
    for (uint32_t i = 0; i < NumBarrierGroups; i++) {
        switch (pBarrierGroups[i].Type) {
            default:
                ASSERT(false, "Unexpected type");
                break;
            case D3D12_BARRIER_TYPE_GLOBAL:
                globalBarrierOffset += pBarrierGroups[i].NumBarriers;
                break;
            case D3D12_BARRIER_TYPE_TEXTURE:
                textureBarrierOffset += pBarrierGroups[i].NumBarriers;
                break;
            case D3D12_BARRIER_TYPE_BUFFER:
                bufferBarrierOffset += pBarrierGroups[i].NumBarriers;
                break;
        }
    }

    // Unwrapped barriers
    TrivialStackVector<D3D12_BARRIER_GROUP, 16> groups;
    TrivialStackVector<D3D12_GLOBAL_BARRIER, 32> globalBarriers;
    TrivialStackVector<D3D12_TEXTURE_BARRIER, 32> textureBarriers;
    TrivialStackVector<D3D12_BUFFER_BARRIER, 32> bufferBarriers;

    // Allocate space
    groups.Resize(NumBarrierGroups);
    globalBarriers.Resize(globalBarrierOffset);
    textureBarriers.Resize(textureBarrierOffset);
    bufferBarriers.Resize(bufferBarrierOffset);

    // Reset counters
    globalBarrierOffset = 0;
    textureBarrierOffset = 0;
    bufferBarrierOffset = 0;

    // Unwrap all objects
    for (uint32_t i = 0; i < NumBarrierGroups; i++) {
        const D3D12_BARRIER_GROUP& source = pBarrierGroups[i];

        // Copy group
        D3D12_BARRIER_GROUP& dest = groups[i];
        dest.Type = source.Type;
        dest.NumBarriers = source.NumBarriers;

        switch (pBarrierGroups[i].Type) {
            case D3D12_BARRIER_TYPE_GLOBAL: {
                // Unwrap global
                for (uint32_t barrierIndex = 0; barrierIndex < source.NumBarriers; barrierIndex++) {
                    globalBarriers[globalBarrierOffset + barrierIndex] = source.pGlobalBarriers[barrierIndex];
                }

                // Invalidate all caching
                HandleGlobalBarrierCacheInvalidation(table.state);
                
                dest.pGlobalBarriers = globalBarriers.Data() + globalBarrierOffset;
                globalBarrierOffset += source.NumBarriers;
                break;
            }
            case D3D12_BARRIER_TYPE_TEXTURE: {
                // Unwrap texture
                for (uint32_t barrierIndex = 0; barrierIndex < source.NumBarriers; barrierIndex++) {
                    D3D12_TEXTURE_BARRIER barrier = source.pTextureBarriers[barrierIndex];
                    auto resourceTable = GetTable(barrier.pResource);

                    // Invalidate dependent caching
                    HandleBarrierCacheInvalidation(table.state, resourceTable.state);
                    
                    barrier.pResource = resourceTable.next;
                    textureBarriers[textureBarrierOffset + barrierIndex] = barrier;
                }
                
                dest.pTextureBarriers = textureBarriers.Data() + textureBarrierOffset;
                textureBarrierOffset += source.NumBarriers;
                break;
            }
            case D3D12_BARRIER_TYPE_BUFFER: {
                // Unwrap buffer
                for (uint32_t barrierIndex = 0; barrierIndex < source.NumBarriers; barrierIndex++) {
                    D3D12_BUFFER_BARRIER barrier = source.pBufferBarriers[barrierIndex];
                    auto resourceTable = GetTable(barrier.pResource);

                    // Invalidate dependent caching
                    HandleBarrierCacheInvalidation(table.state, resourceTable.state);
                    
                    barrier.pResource = resourceTable.next;
                    bufferBarriers[bufferBarrierOffset + barrierIndex] = barrier;
                }
                
                dest.pBufferBarriers = bufferBarriers.Data() + bufferBarrierOffset;
                bufferBarrierOffset += source.NumBarriers;
                break;
            }
        }
    }

    // Validation
    ASSERT(globalBarrierOffset == globalBarriers.Size(), "Invalid offset");
    ASSERT(textureBarrierOffset == textureBarriers.Size(), "Invalid offset");
    ASSERT(bufferBarrierOffset == bufferBarriers.Size(), "Invalid offset");

    // Pass down callchain
    table.bottom->next_Barrier(table.next, static_cast<uint32_t>(groups.Size()), groups.Data());
}

void WINAPI HookID3D12CommandListBeginRenderPass(ID3D12CommandList* list, UINT NumRenderTargets, const D3D12_RENDER_PASS_RENDER_TARGET_DESC* pRenderTargets, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* pDepthStencil, D3D12_RENDER_PASS_FLAGS Flags) {
    auto table = GetTable(list);

    // Copy all state data
    ShaderExportRenderPassState& renderPassState = table.state->streamState->renderPass;
    renderPassState.insideRenderPass = true;
    renderPassState.renderTargetCount = NumRenderTargets;
    renderPassState.flags = Flags;

    // Copy render targets
    ASSERT(NumRenderTargets <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, "Exceeding max number of render targets");
    std::memcpy(renderPassState.renderTargets, pRenderTargets, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * NumRenderTargets);

    // Depth is optional
    if (pDepthStencil) {
        renderPassState.depthStencil = *pDepthStencil;
    } else {
        renderPassState.depthStencil.cpuDescriptor.ptr = 0ull;
    }

    // Start the user pass
    BeginRenderPassForUser(table.next, &renderPassState);
}

void WINAPI HookID3D12CommandListEndRenderPass(ID3D12CommandList* list) {
    auto table = GetTable(list);

    // Mark as outside
    ASSERT(table.state->streamState->renderPass.insideRenderPass, "Unexpected render pass state");
    table.state->streamState->renderPass.insideRenderPass = false;
    
    // Pass down callchain
    table.next->EndRenderPass();

    // Handle any pending operations
    ResolveRenderPassForUserEnd(table.next, &table.state->streamState->renderPass);
}

static uint32_t AllocateRollingUID(DeviceState* state) {
    // Allocate the identifier, we never want zero as that's reserved
    for (;;) {
        if (uint32_t rollingUID = state->rollingUIDs++; rollingUID != 0) {
            return rollingUID;
        }
    }
}

void WINAPI HookID3D12CommandListRSSetViewports(ID3D12CommandList *list, UINT NumViewports, const D3D12_VIEWPORT *pViewports) {
    auto table = GetTable(list);

    if (NumViewports) {
        table.state->streamState->viewport.state = pViewports[0];
        table.state->streamState->viewport.rollingUID = AllocateRollingUID(GetState(table.state->parent));
    }

    // Pass down callchain
    table.next->RSSetViewports(NumViewports, pViewports);
}

void ReconstructPipelineState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState) {
    ShaderExportStreamBindState &bindState = streamState->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Reset signature if needed
    if (bindState.rootSignature) {
        commandList->SetComputeRootSignature(bindState.rootSignature->object);
    }

    // Existing pipeline?
    IUnknown* pipelineObject{nullptr};
    if (streamState->pipelineObject) {
        pipelineObject = streamState->pipelineObject;
    } else if (streamState->pipeline) {
        pipelineObject = streamState->pipeline->object;
    }

    // Bind based on type
    if (pipelineObject) {
        if (streamState->pipeline->type == PipelineType::StateObject) {
            // TODO[rt]: Cast is unsafe!
            static_cast<ID3D12GraphicsCommandList4*>(commandList)->SetPipelineState1(static_cast<ID3D12StateObject *>(pipelineObject));
        } else {
            commandList->SetPipelineState(static_cast<ID3D12PipelineState *>(pipelineObject));
        }
    }
    
    // Reset root data if needed, invalidated by signature change
    if (bindState.rootSignature) {
        for (uint32_t i = 0; i < bindState.rootSignature->logicalMapping.userRootCount; i++) {
            const ShaderExportRootParameterValue &value = bindState.persistentRootParameters[i];

            // Get the expected heap type
            D3D12_DESCRIPTOR_HEAP_TYPE heapType = bindState.rootSignature->logicalMapping.userRootMappings[i].heapType;
            
            switch (value.type) {
                case ShaderExportRootParameterValueType::None: {
                    break;
                }
                case ShaderExportRootParameterValueType::Descriptor: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Unexpected heap type");
                    commandList->SetComputeRootDescriptorTable(i, value.payload.descriptor);
                    break;
                }
                case ShaderExportRootParameterValueType::SRV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootShaderResourceView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::UAV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootUnorderedAccessView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::CBV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootConstantBufferView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::Constant: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Unexpected heap type");
                    commandList->SetComputeRoot32BitConstants(
                        i,
                        value.payload.constant.dataByteCount / sizeof(uint32_t),
                        value.payload.constant.data,
                        0
                    );
                    break;
                }
            }
        }

        // Compute overwritten at this point
        streamState->pipelineSegmentMask &= ~PipelineTypeSet(PipelineType::Compute);

        // Rebind the export, invalidated by signature change
        if (streamState->pipeline) {
            device->exportStreamer->BindShaderExport(streamState, streamState->pipeline, commandList);
        }
    }
}

void ReconstructRenderPassState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState) {
    BeginRenderPassForReconstruction(static_cast<ID3D12GraphicsCommandList4*>(commandList), &streamState->renderPass);
}

void ReconstructHeapState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState) {
    TrivialStackVector<ID3D12DescriptorHeap*, 2u> heaps;

    if (streamState->resourceHeap) {
        heaps.Add(streamState->resourceHeap->object);
    }
    
    if (streamState->samplerHeap) {
        heaps.Add(streamState->samplerHeap->object);
    }

    commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.Size()), heaps.Data());
}

void ReconstructState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState, ReconstructionFlagSet flags) {
    if (flags & ReconstructionFlag::Heap) {
        ReconstructHeapState(device, commandList, streamState);
    }
    
    if (flags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(device, commandList, streamState);
    }

    if (flags & ReconstructionFlag::RenderPass) {
        ReconstructRenderPassState(device, commandList, streamState);
    }
}

void ReconstructState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState) {
    ReconstructionFlagSet flags = ReconstructionFlag::Pipeline | ReconstructionFlag::RootConstant | ReconstructionFlag::Heap;

    if (streamState->renderPass.insideRenderPass) {
        flags |= ReconstructionFlag::RenderPass;
    }
    
    ReconstructState(device, commandList, streamState, flags);
}

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif // _MSC_VER

struct ALIGN_PACK PIXEvent3Header {
    uint64_t encoding;
    uint64_t color;
    uint64_t formatEncoding;
    wchar_t  format[1];
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif // _MSC_VER

static void DecodeAndPushMarker(ShaderExportStreamMarkerState& markers, UINT Metadata, const void *pData, UINT Size, bool hierarchical ) {
    switch (Metadata) {
        default: {
            // Not implemented
            break;
        }
        case WINPIX_EVENT_ANSI_VERSION: {
            // Just a plain string
            markers.stack.Add(ShaderExportStreamMarkerEntryState {
                .hierarchical = hierarchical,
                .hash32 = BufferCRC32Short(pData, Size)
            });
            break;
        }
        case WINPIX_EVENT_UNICODE_VERSION: {
            const wchar_t* source = static_cast<const wchar_t*>(pData);
            char*          dest   = ALLOCA_ARRAY(char, Size / sizeof(wchar_t));

            // Convert
            size_t length = std::wcslen(static_cast<const wchar_t *>(pData)) + 1;
            wcstombs_s(&length, dest, length * sizeof(char), source, length * sizeof(wchar_t));

            // Just a plain string
            markers.stack.Add(ShaderExportStreamMarkerEntryState {
                .hierarchical = hierarchical,
                .hash32 = BufferCRC32Short(dest, Size)
            });
            break;
        }
        case D3D12_EVENT_METADATA: {
            auto* dataU8 = static_cast<const uint8_t*>(pData);
            auto* endU8  = dataU8 + Size;
            if (Size < sizeof(PIXEvent3Header)) {
                return;
            }

            // Always header
            auto* header = reinterpret_cast<const PIXEvent3Header*>(dataU8);
            dataU8 += sizeof(PIXEvent3Header);

            // Get format
            const wchar_t* format = header->format;
            size_t formatLength = std::wcslen(format);
            dataU8 += formatLength * sizeof(wchar_t);

            // To Qword
            if (reinterpret_cast<uint64_t>(dataU8) % sizeof(uint64_t)) {
                dataU8 += sizeof(uint64_t) - reinterpret_cast<uint64_t>(dataU8) % sizeof(uint64_t);
            }

            // Get all arguments
            TrivialStackVector<const wchar_t*, 16> args;
            while (dataU8 < endU8 && (endU8 - dataU8) >= sizeof(uint64_t) + sizeof(wchar_t)) {
                uint64_t encoding = *reinterpret_cast<const uint64_t*>(dataU8);
                dataU8 += sizeof(uint64_t);

                // Get argument
                const wchar_t* argBegin = reinterpret_cast<const wchar_t*>(dataU8);
                args.Add(argBegin);

                // Skip
                size_t lengthTerm = std::wcslen(argBegin) + 1;
                dataU8 += lengthTerm * sizeof(wchar_t);

                // To Qword
                if (reinterpret_cast<uint64_t>(dataU8) % sizeof(uint64_t)) {
                    dataU8 += sizeof(uint64_t) - reinterpret_cast<uint64_t>(dataU8) % sizeof(uint64_t);
                }
            }

            // Current argument offset
            uint32_t argumentOffset = 0;

            // Format the text
            TrivialStackVector<char, 1024> formatted;
            for (uint32_t i = 0; i < formatLength; i++) {
                if (format[i] != '%' || i >= formatLength - 1) {
                    formatted.Add(static_cast<char>(format[i]));
                    continue;
                }

                i++;

                // Bad format?
                if (argumentOffset >= args.Size()) {
                    break;
                }

                // Handle formatting
                switch (format[i]) {
                    case L's':
                    case L'S': {
                        uint64_t len = std::wcslen(args[argumentOffset]);
                        formatted.Reserve(formatted.Size() + len);

                        for (uint32_t argChIndex = 0; argChIndex < len; argChIndex++) {
                            formatted.Add(static_cast<char>(args[argumentOffset][argChIndex]));
                        }
                        break;
                    }
                }
            }

            // Just a plain string
            markers.stack.Add(ShaderExportStreamMarkerEntryState {
                .hierarchical = hierarchical,
                .hash32 = BufferCRC32Short(formatted.Data(), formatted.Size())
            });
            break;
        }
    }
}

void WINAPI HookID3D12CommandListSetMarker(ID3D12CommandList *list, UINT Metadata, const void *pData, UINT Size) {
    auto table = GetTable(list);

    // Pass down
    table.next->SetMarker(Metadata, pData, Size);
    
    // Accommodation check
    ShaderExportStreamMarkerState &markers = table.state->streamState->markers;
    if (markers.stack.Size()) {
        ShaderExportStreamMarkerEntryState &last = markers.stack[markers.stack.Size() - 1];

        // If hierarchical and out of space? Bail.
        if (!last.hierarchical && markers.stack.Size() >= kMaxExecutionInfoMarkerCount) {
            return;
        }
    }

    // Actually decode it
    DecodeAndPushMarker(markers, Metadata, pData, Size, false);
}

void WINAPI HookID3D12CommandListBeginEvent(ID3D12CommandList *list, UINT Metadata, const void *pData, UINT Size) {
    auto table = GetTable(list);

    // Pass down
    table.next->BeginEvent(Metadata, pData, Size);
    
    // Accommodation check
    ShaderExportStreamMarkerState &markers = table.state->streamState->markers;
    if (markers.stack.Size() >= kMaxExecutionInfoMarkerCount) {
        return;
    }

    // Actually decode it
    DecodeAndPushMarker(markers, Metadata, pData, Size, true);
}

void WINAPI HookID3D12CommandListEndEvent(ID3D12CommandList *list) {
    auto table = GetTable(list);

    // Pass down
    table.next->EndEvent();

    // Accommodation check
    ShaderExportStreamMarkerState &markers = table.state->streamState->markers;
    if (!markers.stack.Size()) {
        return;
    }

    // If non-hierarchical, pop that too  
    ShaderExportStreamMarkerEntryState &last = markers.stack[markers.stack.Size() - 1];
    if (!last.hierarchical) {
        markers.stack.PopBack();
    }

    // Pop the hierarchical event
    if (markers.stack.Size()) {
        markers.stack.PopBack();
    }
}

static ExecutionInfo GetBaseExecutionInfo(CommandListState* state) {
    DeviceState* device = GetState(state->parent);

    // Default info
    ExecutionInfo info{};

    // Allocate the identifier, we never want zero as that's reserved
    info.rollingExecutionUID = AllocateRollingUID(device);
    info.rollingViewportUID = state->streamState->viewport.rollingUID;

    // Pipeline is optional
    info.pipelineUID = state->streamState->pipeline ? static_cast<uint32_t>(state->streamState->pipeline->uid) : 0;

    // Fill marker hashes
    for (uint32_t i = 0; i < kMaxExecutionInfoMarkerCount; i++) {
        if (i < state->streamState->markers.stack.Size()) {
            info.markerHashes32[i] = state->streamState->markers.stack[i].hash32;
        } else {
            info.markerHashes32[i] = 0;
        }
    }

    // Set the viewport
    info.viewport.width = static_cast<uint32_t>(state->streamState->viewport.state.Width);
    info.viewport.height = static_cast<uint32_t>(state->streamState->viewport.state.Height);

    // OK
    return info;
}

void CommitGraphics(DeviceState* device, CommandListState* list) {
    // Commit all commands prior to binding
    CommitCommands(list);

    // Inform the streamer
    device->exportStreamer->CommitGraphics(list->streamState, list->object);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = list->userContext.eventStack.GetGraphicsDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            list->object->SetGraphicsRoot32BitConstant(
                list->streamState->pipeline->signature->logicalMapping.userRootCount + 2u,
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

void CommitCompute(DeviceState* device, CommandListState* list) {
    // Commit all commands prior to binding
    CommitCommands(list);

    // Inform the streamer
    device->exportStreamer->CommitCompute(list->streamState, list->object);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = list->userContext.eventStack.GetComputeDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            list->object->SetComputeRoot32BitConstant(
                list->streamState->pipeline->signature->logicalMapping.userRootCount + 2u,
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

static bool UsesExecutionInfo(CommandListState* state) {
    // No instrument, no execution info
    if (!state->streamState->pipelineInstrument) {
        return false;
    }

    // Check if any shader in the pipeline uses execution info
    return state->streamState->pipelineInstrument->featureTable.executionInfo;
}

void WINAPI HookID3D12CommandListDrawInstanced(ID3D12CommandList* list, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Append execution info, if used
    if (UsesExecutionInfo(table.state)) {
        ExecutionInfo info = GetBaseExecutionInfo(table.state);
        info.executionFlags = ExecutionFlag::TypeDraw;
        info.draw.drawFlags = ExecutionDrawFlag::VertexCountPerInstance | ExecutionDrawFlag::InstanceCount | ExecutionDrawFlag::StartVertex | ExecutionDrawFlag::StartInstance;
        info.draw.vertexCountPerInstance = VertexCountPerInstance;
        info.draw.instanceCount = InstanceCount;
        info.draw.startVertex = StartVertexLocation;
        info.draw.startInstance = StartInstanceLocation;
        device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Graphics, info);
    }
    
    // Commit all pending graphics
    CommitGraphics(device.state, table.state);

    // Pass down callchain
    table.next->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void WINAPI HookID3D12CommandListDrawIndexedInstanced(ID3D12CommandList* list, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Append execution info, if used
    if (UsesExecutionInfo(table.state)) {
        ExecutionInfo info = GetBaseExecutionInfo(table.state);
        info.executionFlags = ExecutionFlag::TypeDraw;
        info.draw.drawFlags = ExecutionDrawFlag::IndexCountPerInstance | ExecutionDrawFlag::InstanceCount | ExecutionDrawFlag::StartVertex | ExecutionDrawFlag::StartIndex;
        info.draw.indexCountPerInstance = IndexCountPerInstance;
        info.draw.instanceCount = InstanceCount;
        info.draw.instanceOffset = StartInstanceLocation;
        info.draw.startIndex = StartIndexLocation;
        info.draw.startVertex = BaseVertexLocation;
        device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Graphics, info);
    }

    // Commit all pending graphics
    CommitGraphics(device.state, table.state);

    // Pass down callchain
    table.next->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

void WINAPI HookID3D12CommandListDispatch(ID3D12CommandList* list, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);
    
    // Append execution info, if used
    if (UsesExecutionInfo(table.state)) {
        ExecutionInfo info = GetBaseExecutionInfo(table.state);
        info.executionFlags = ExecutionFlag::TypeDispatch;
        info.dispatch.groupCountX = ThreadGroupCountX;
        info.dispatch.groupCountY = ThreadGroupCountY;
        info.dispatch.groupCountZ = ThreadGroupCountZ;
        device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Compute, info);
    }

    // Commit all pending compute
    CommitCompute(device.state, table.state);

    // Pass down callchain
    table.next->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

void WINAPI HookID3D12CommandListDispatchMesh(ID3D12CommandList* list, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);
    
    // Append execution info, if used
    if (UsesExecutionInfo(table.state)) {
        ExecutionInfo info = GetBaseExecutionInfo(table.state);
        info.executionFlags = ExecutionFlag::TypeDraw;
        device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Graphics, info);
    }

    // Commit all pending graphics
    CommitGraphics(device.state, table.state);

    // Pass down callchain
    table.next->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
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
    device.state->exportStreamer->SetComputeRootConstants(table.state->streamState, RootParameterIndex, pSrcData, sizeof(UINT) * Num32BitValuesToSet, DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetComputeRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListSetGraphicsRoot32BitConstants(ID3D12CommandList *list, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void *pSrcData, UINT DestOffsetIn32BitValues) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Inform the streamer
    device.state->exportStreamer->SetGraphicsRootConstants(table.state->streamState, RootParameterIndex, pSrcData, sizeof(UINT) * Num32BitValuesToSet, DestOffsetIn32BitValues);

    // Pass down call chain
    table.next->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValuesToSet, pSrcData, DestOffsetIn32BitValues);
}

void WINAPI HookID3D12CommandListExecuteIndirect(ID3D12CommandList* list, ID3D12CommandSignature *pCommandSignature, UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset, ID3D12Resource *pCountBuffer, UINT64 CountBufferOffset) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get signature
    auto signatureTable = GetTable(pCommandSignature);

    // Append execution info, if used
    // TODO[dbg]: The execution info has to be filled in GPU-side, we can't know the payloads here
    if (UsesExecutionInfo(table.state)) {
        ExecutionInfo info = GetBaseExecutionInfo(table.state);
        info.executionFlags = ExecutionFlag::TypeIndirect;
        
        switch (table.state->streamState->pipeline->type) {
            default:
                ASSERT(false, "Unexpected state");
                break;
            case PipelineType::Graphics:
                info.executionFlags |= ExecutionFlag::TypeDraw;
                break;
            case PipelineType::Compute:
                info.executionFlags |= ExecutionFlag::TypeDispatch;
                break;
            case PipelineType::StateObject:
                // TODO[dbg]: Won't hold true with WG
                info.executionFlags |= ExecutionFlag::TypeRaytracing;
                break;
        }
        
        // Set compute info if needed
        if (signatureTable.state->activeTypes & PipelineType::Compute) {
            device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Compute, info);
        }

        // Set graphics info if needed
        if (signatureTable.state->activeTypes & PipelineType::Graphics) {
            device.state->exportStreamer->SetExecutionInfo(table.state->streamState, PipelineType::Graphics, info);
        }
    }

    // If there's a proxy, we need to create the destination arguments
    if (table.state->proxies.featureBitSet_ExecuteIndirect) {
        // Effective length of the commands
        uint32_t commandByteLength = signatureTable.state->byteStride * MaxCommandCount;
        
        // Create dest command allocation
        ShaderExportDeviceAllocation destAllocation = table.state->streamState->deviceAllocator.Allocate(device.state->deviceAllocator, commandByteLength);

        // Transient resource state
        ResourceState destState {
            .object = destAllocation.allocation.resource,
            .desc = destAllocation.allocation.resource->GetDesc(),
            .uid = kResourceUIDTransient
        };

        // Allocate new PUID's for lookups
        destState.virtualMapping.token.puid = device.state->physicalResourceIdentifierMap.AllocatePUID(&destState);

        // Source Indirect -> Copy
        // TODO[cmd]: This is incorrect, it could be masked
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = Next(pArgumentBuffer);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        table.next->ResourceBarrier(1u, &barrier);

        // Copy over the source commands
        table.next->CopyBufferRegion(
            destAllocation.allocation.resource, 0,
            Next(pArgumentBuffer), ArgumentBufferOffset,
            commandByteLength
        );

        // Source Copy -> Indirect
        barrier.Transition.pResource = Next(pArgumentBuffer);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        table.next->ResourceBarrier(1u, &barrier);

        // Dest Copy -> Unordered
        barrier.Transition.pResource = destAllocation.allocation.resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        table.next->ResourceBarrier(1u, &barrier);

        // Invoke proxies
        if (ApplyFeatureHook<FeatureHook_ExecuteIndirect>(
            table.state,
            table.state->proxies.context,
            table.state->proxies.featureBitSet_ExecuteIndirect,
            table.state->proxies.featureHooks_ExecuteIndirect,
            pCommandSignature, MaxCommandCount, pArgumentBuffer,
            ArgumentBufferOffset, pCountBuffer, CountBufferOffset,
            &destState
        )) {
            CommitCommands(table.state);
        }

        // Unordered -> Indirect
        barrier.Transition.pResource = destAllocation.allocation.resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        table.next->ResourceBarrier(1u, &barrier);

        // Free the transient PUID's
        device.state->physicalResourceIdentifierMap.FreePUID(destState.virtualMapping.token.puid);

        // Overwrite the argument buffer
        // Subsequent patching/instrumentation will work on the proxy-modified arguments
        pArgumentBuffer      = destAllocation.allocation.resource;
        ArgumentBufferOffset = 0;
    }

    // State object (raytracing) EI's require patching
    if (signatureTable.state->activeTypes & PipelineType::StateObject) {
        // Let the indirect raytracing handle it
        HookID3D12CommandListExecuteIndirectRaytracing(list, pCommandSignature, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset, pCountBuffer, CountBufferOffset);
    } else {
        // Commit compute if needed
        if (signatureTable.state->activeTypes & PipelineType::Compute) {
            device.state->exportStreamer->CommitCompute(table.state->streamState, table.state->object);
        }
    
        // Commit graphics if needed
        if (signatureTable.state->activeTypes & PipelineType::Graphics) {
            device.state->exportStreamer->CommitGraphics(table.state->streamState, table.state->object);
        }
        
        // No patching, just pass down callchain
        table.next->ExecuteIndirect(
            signatureTable.next, 
            MaxCommandCount, 
            ConditionalNext(pArgumentBuffer),
            ArgumentBufferOffset, 
            Next(pCountBuffer), 
            CountBufferOffset
        );
    }
}

void WINAPI HookID3D12CommandListCopyTiles(ID3D12CommandList *list, ID3D12Resource* pTiledResource, const D3D12_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D12_TILE_REGION_SIZE* pTileRegionSize, ID3D12Resource* pBuffer, UINT64 BufferStartOffsetInBytes, D3D12_TILE_COPY_FLAGS Flags) {
    auto table = GetTable(list);
    
    // Pass down callchain
    table.next->CopyTiles(
        Next(pTiledResource),
        pTileRegionStartCoordinate,
        pTileRegionSize,
        Next(pBuffer),
        BufferStartOffsetInBytes,
        Flags
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

    // Commit all pending commands prior to closing
    CommitCommands(table.state);
    
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

    // Get pipeline
    PipelineState* pipelineState = GetState(pipeline);
    ASSERT(pipelineState->type == PipelineType::Graphics || pipelineState->type == PipelineType::Compute, "Unexpected pipeline state");
    
    // Get hot swap
    PipelineInstrument *instrument = pipelineState->hotSwapObject.load();

    // Conditionally wait for instrumentation if the pipeline has an outstanding request
    if (!instrument && pipelineState->HasInstrumentationRequest()) {
        device.state->instrumentationController->ConditionalWaitForCompletion();

        // Load new hot-object
        instrument = pipelineState->hotSwapObject.load();
    }

    // Active pipeline
    ID3D12PipelineState* pipelineObject = instrument ? static_cast<ID3D12PipelineState*>(instrument->object) : Next(pipeline);

    // Update last used
    pipelineState->lastUsedTimestampNS.store(device.state->syncPointActionThread.GetLastTimeSinceEpochNS(), std::memory_order::relaxed);

    // Pass down callchain
    table.bottom->next_SetPipelineState(table.next, pipelineObject);

    // Inform the streamer of a new pipeline
    device.state->exportStreamer->BindPipeline(table.state->streamState, pipelineState, pipelineObject, instrument, table.state->object);
}

void WINAPI HookID3D12CommandListSetPipelineState1(ID3D12CommandList *list, ID3D12StateObject *stateObject) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get state
    StateObjectState* stateObjectState = GetState(stateObject);
    ASSERT(stateObjectState->type == PipelineType::StateObject, "Unexpected state object state");
    
    // Get hot swap
    PipelineInstrument *instrument = stateObjectState->hotSwapObject.load();

    // Conditionally wait for instrumentation if the pipeline has an outstanding request
    if (!instrument && stateObjectState->HasInstrumentationRequest()) {
        device.state->instrumentationController->ConditionalWaitForCompletion();

        // Load new hot-object
        instrument = stateObjectState->hotSwapObject.load();
    }

    // Active pipeline
    ID3D12StateObject* pipelineObject = instrument ? static_cast<ID3D12StateObject*>(instrument->object) : Next(stateObject);

    // Update last used
    stateObjectState->lastUsedTimestampNS.store(device.state->syncPointActionThread.GetLastTimeSinceEpochNS(), std::memory_order::relaxed);

    // Pass down callchain
    table.bottom->next_SetPipelineState1(table.next, pipelineObject);

    // Inform the streamer of a new pipeline
    device.state->exportStreamer->BindPipeline(table.state->streamState, stateObjectState, pipelineObject, instrument, table.state->object);
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

static void InvokePreSubmitBatchHook(const CommandQueueTable& table, const DeviceTable& device, ShaderExportStreamSegment* segment, CommandContextHandle* handles, uint32_t commandListCount, SubmissionContext& context) {
    ShaderExportStreamSegmentUserContext& preContext = segment->userPreContext;
    ShaderExportStreamSegmentUserContext& postContext = segment->userPostContext;
    
    // Reset pre-context
    preContext.commandContext.eventStack.Flush();
    preContext.commandContext.eventStack.SetRemapping(device.state->eventRemappingTable);
    preContext.commandContext.handle = reinterpret_cast<CommandContextHandle>(&segment->userPreContext);
    preContext.commandContext.queueHandle = reinterpret_cast<CommandQueueHandle>(table.state->object);
    
    // Reset post-context
    postContext.commandContext.eventStack.Flush();
    postContext.commandContext.eventStack.SetRemapping(device.state->eventRemappingTable);
    postContext.commandContext.handle = reinterpret_cast<CommandContextHandle>(&segment->userPostContext);
    postContext.commandContext.queueHandle = reinterpret_cast<CommandQueueHandle>(table.state->object);

    // Setup hook contexts
    context.preContext = &preContext.commandContext;
    context.postContext = &postContext.commandContext;
    
    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: device.state->featureHookTables) {
        proxyTable.preSubmit.TryInvoke(context, handles, commandListCount);
    }
}

static ID3D12GraphicsCommandList* RecordExecutePreCommandList(const CommandQueueTable& table, const DeviceTable& device, ShaderExportStreamSegment* segment) {
    ShaderExportStreamSegmentUserContext& context = segment->userPreContext;

    // Record the streaming pre patching
    ID3D12GraphicsCommandList* patchList = device.state->exportStreamer->RecordPreCommandList(table.state, segment);

    // Any commands?
    if (context.commandContext.buffer.Count()) {
        // Lazy allocate streaming state
        if (!context.streamState) {
            context.streamState = device.state->exportStreamer->AllocateStreamState();
        }
        
        // Open the streamer state
        device.state->exportStreamer->BeginCommandList(context.streamState, patchList);
        {
            // Commit all commands
            CommitCommands(
                device.state,
                patchList,
                context.commandContext.buffer,
                context.streamState
            );
        
            // Close the streamer state
            device.state->exportStreamer->CloseCommandList(context.streamState);
        }
        
        // Clear all commands
        context.commandContext.buffer.Clear();
        
        // Create streamer allocation association
        device.state->exportStreamer->MapSegment(context.streamState, patchList, segment);
    }

    // Done
    HRESULT hr = patchList->Close();
    if (FAILED(hr)) {
        return nullptr;
    }
    
    return patchList;
}

static ID3D12GraphicsCommandList* RecordExecutePostCommandList(const CommandQueueTable& table, const DeviceTable& device, ShaderExportStreamSegment* segment) {
    ShaderExportStreamSegmentUserContext& context = segment->userPostContext;

    // Record the streaming pre patching
    ID3D12GraphicsCommandList* patchList = device.state->exportStreamer->RecordPostCommandList(table.state, segment);

    // Any commands?
    if (context.commandContext.buffer.Count()) {
        // Lazy allocate streaming state
        if (!context.streamState) {
            context.streamState = device.state->exportStreamer->AllocateStreamState();
        }
        
        // Open the streamer state
        device.state->exportStreamer->BeginCommandList(context.streamState, patchList);
        {
            // Commit all commands
            CommitCommands(
                device.state,
                patchList,
                context.commandContext.buffer,
                context.streamState
            );
        
            // Close the streamer state
            device.state->exportStreamer->CloseCommandList(context.streamState);
        }
        
        // Clear all commands
        context.commandContext.buffer.Clear();
        
        // Create streamer allocation association
        device.state->exportStreamer->MapSegment(context.streamState, patchList, segment);
    }

    // Done
    HRESULT hr = patchList->Close();
    if (FAILED(hr)) {
        return nullptr;
    }

    return patchList;
}

void HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue *queue, UINT count, ID3D12CommandList *const *lists) {
    auto table = GetTable(queue);

    // Special case, allow native submissions on wrapped objects
    // Also helps hierarchical hooking.
    if (count && !IsWrapped(lists[0])) {
        table.next->ExecuteCommandLists(count, lists);
        return;   
    }

    // Get device
    auto device = GetTable(table.state->parent);

    // Flush any pending work
    FlushCommandQueueContext(GetState(table.state->parent), table.state);

    // Special case, invoke a device sync point during empty submissions
    if (count == 0) {
        BridgeDeviceSyncPoint(device.state, table.state);
    }

    // Process all pending descriptor states
    // To reduce general (heap) register pressure, we check the pending completion states independently of the existing lifetime,
    // latter which is tied to the command allocators. This greatly helps reduce the amount of concurrent descriptor states,
    // and avoid exhausted heap issues.
    device.state->exportStreamer->ProcessDescriptors();

    // Allocate submission segment
    ShaderExportStreamSegment* segment = device.state->exportStreamer->AllocateSegment();
    
    // Inform the controller of the segmentation point
    segment->versionSegPoint = device.state->versioningController->BranchOnSegmentationPoint();

    // Final list of commands to submit
    TrivialStackVector<ID3D12CommandList*, 64u> unwrapped;
    TrivialStackVector<CommandContextHandle, 64> contextHandles;

    // Reserve a slot for the streaming pre patching
    unwrapped.Add(nullptr);

    // Process all lists
    for (uint32_t i = 0; i < count; i++) {
        auto listTable = GetTable(lists[i]);

        // Create streamer allocation association
        device.state->exportStreamer->MapSegment(listTable.state->streamState, listTable.next, segment);

        // Pass down unwrapped
        unwrapped.Add(listTable.next);

        // Keep track of handle
        contextHandles.Add(reinterpret_cast<CommandContextHandle>(listTable.state->streamState));
    }

    // Run the submission hook
    SubmissionContext submitContext;
    InvokePreSubmitBatchHook(table, device, segment, contextHandles.Data(), count, submitContext);

    // Record the streaming pre patching
    unwrapped[0] = RecordExecutePreCommandList(table, device, segment);

    // Record the streaming post patching
    unwrapped.Add(RecordExecutePostCommandList(table, device, segment));

    // Wait for all primitives
    for (const SchedulerPrimitiveEvent& primitive : submitContext.waitPrimitives) {
        table.next->Wait(device.state->scheduler->GetPrimitiveFence(primitive.id), primitive.value);
    } 

    // Pass down callchain
    table.bottom->next_ExecuteCommandLists(table.next, static_cast<uint32_t>(unwrapped.Size()), unwrapped.Data());

    // Signal for all primitives
    for (const SchedulerPrimitiveEvent& primitive : submitContext.signalPrimitives) {
        table.next->Signal(device.state->scheduler->GetPrimitiveFence(primitive.id), primitive.value);
    } 

    // Run post submission for all proxies
    for (const FeatureHookTable &proxyTable: device.state->featureHookTables) {
        proxyTable.postSubmit.TryInvoke(contextHandles.Data(), count);
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

    // Release parent
    parent->Release();
}

CommandAllocatorState::~CommandAllocatorState() {
    auto device = GetTable(parent);

    // Cleanup references to all command lists
    for (CommandListState *commandList : commandLists) {
        commandList->owningAllocator = nullptr;
        commandList->allocatorSlotId = kInvalidSlotId;

        // Lifetime is always that of the allocator
        commandList->streamState = nullptr;
    }
    
    // The allocator owns the lifetimes of the streaming states
    // If an allocator has been destroyed, all the submissions are done, so free the states up
    for (ShaderExportStreamState *streamState : streamStates) {
        device.state->exportStreamer->Free(streamState);
    }
}

CommandListState::~CommandListState() {
    auto device = GetTable(parent);

    // The command lists do not own the streaming states, leave them be
    ASSERT(!streamState || owningAllocator, "Dangling streaming state");

    // Remove from owning allocator
    if (allocatorSlotId != kInvalidSlotId) {
        auto owningTable = GetTable(owningAllocator);

        // Serial
        std::lock_guard guard(owningTable.state->lock);
        owningTable.state->commandLists.Remove(this);
    }
}

ImmediateCommandList CommandQueueState::PopCommandList() {
    std::lock_guard guard(mutex);
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
    std::lock_guard guard(mutex);
    
    HRESULT hr = list.allocator->Reset();
    ASSERT(SUCCEEDED(hr), "Pushing in-flight immediate command list");

    // Append
    commandLists.push_back(list);
}
