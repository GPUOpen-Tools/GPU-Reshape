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

#include <Backends/DX12/Resource.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Controllers/VersioningController.h>
#include <Backends/DX12/Translation.h>

// Backend
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/IL/ResourceTokenType.h>

ResourceInfo GetResourceInfoFor(ResourceState* state) {
    // Construct without descriptor
    switch (static_cast<Backend::IL::ResourceTokenType>(state->virtualMapping.token.type)) {
        default:
            ASSERT(false, "Unexpected type");
            return {};
        case Backend::IL::ResourceTokenType::Texture:
            return ResourceInfo::Texture(state->virtualMapping.token, state->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D);
        case Backend::IL::ResourceTokenType::Buffer:
            return ResourceInfo::Buffer(state->virtualMapping.token);
    }
}

HRESULT HookID3D12ResourceMap(ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* readRange, void** blob) {
    auto table = GetTable(resource);

    // Get device
    auto deviceTable = GetTable(table.state->parent);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Map(table.next, subresource, readRange, blob);
    if (FAILED(hr)) {
        return hr;
    }

    // Get proxy info
    ResourceInfo proxyResourceInfo = GetResourceInfoFor(table.state);

    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: deviceTable.state->featureHookTables) {
        proxyTable.mapResource.TryInvoke(proxyResourceInfo);
    }

    // OK
    return S_OK;
}

HRESULT HookID3D12ResourceUnmap(ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* writtenRange) {
    auto table = GetTable(resource);

    // Get device
    auto deviceTable = GetTable(table.state->parent);

    // Pass down callchain
    table.bottom->next_Unmap(table.next, subresource, writtenRange);

    // Get proxy info
    ResourceInfo proxyResourceInfo = GetResourceInfoFor(table.state);

    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: deviceTable.state->featureHookTables) {
        proxyTable.unmapResource.TryInvoke(proxyResourceInfo);
    }

    // OK
    return S_OK;
}

static ID3D12Resource* CreateResourceState(ID3D12Device* parent, const DeviceTable& table, ID3D12Resource* resource, const D3D12_RESOURCE_DESC* desc, const ResourceCreateFlagSet& createFlags = {}) {
    // Create state
    auto* state = new (table.state->allocators, kAllocStateResource) ResourceState();
    state->allocators = table.state->allocators;
    state->object = resource;
    state->desc = *desc;
    state->parent = parent;
    state->isEmulatedComitted = (createFlags & ResourceCreateFlag::MetadataRequiresHardwareClear);

    // Keep reference
    parent->AddRef();

    // Add state
    table.state->states_Resources.Add(state);

    // Allocate PUID
    state->virtualMapping.token.puid = table.state->physicalResourceIdentifierMap.AllocatePUID(state);

    // Translate dimension
    switch (desc->Dimension) {
        default:
            ASSERT(false, "Unsupported dimension");
            break;
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            state->virtualMapping.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            state->virtualMapping.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
            break;
    }

    // Resource information
    state->virtualMapping.token.formatId = static_cast<uint32_t>(Translate(desc->Format));
    state->virtualMapping.token.formatSize = GetFormatByteSize(desc->Format);
    state->virtualMapping.token.width = static_cast<uint32_t>(desc->Width);
    state->virtualMapping.token.height = desc->Height;
    state->virtualMapping.token.depthOrSliceCount = desc->DepthOrArraySize;
    state->virtualMapping.token.mipCount = desc->MipLevels;

    // Special case, report R1 as "0" (bitwise)
    if (desc->Format == DXGI_FORMAT_R1_UNORM || desc->Format == DXGI_FORMAT_UNKNOWN) {
        state->virtualMapping.token.formatSize = 0;
    }

    // If the number of mips is zero, its automatically deduced
    if (state->virtualMapping.token.mipCount == 0) {
        uint32_t maxDimension = std::max<uint32_t>({static_cast<uint32_t>(desc->Width), desc->Height, desc->DepthOrArraySize});
        state->virtualMapping.token.mipCount = static_cast<uint32_t>(std::floor(std::log2(maxDimension))) + 1u;

        // May be pooled later, update original description
        state->desc.MipLevels = state->virtualMapping.token.mipCount;
    }
    
    // Assume default view
    state->virtualMapping.token.DefaultViewToRange();

    // Create mapping
    switch (desc->Dimension) {
        default:
            break;
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            table.state->virtualAddressTable.Add(state, resource->GetGPUVirtualAddress(), desc->Width);
            break;
    }

    // Inform controller
    table.state->versioningController->CreateOrRecommitResource(state, nullptr);

    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table.state->featureHookTables) {
        proxyTable.createResource.TryInvoke(ResourceCreateInfo {
            .resource = GetResourceInfoFor(state),
            .createFlags = createFlags
        });
    }
    
    // Create detours
    return CreateDetour(state->allocators, resource, state);
}

HRESULT HookID3D12DeviceCreateCommittedResource(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE* clearValue, const IID& riid, void** pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommittedResource(table.next, heap, heapFlags, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(device, table, resource, desc);

    // Query to external object if requested
    if (pResource) {
        hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateCommittedResource1(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommittedResource1(table.next, pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(device, table, resource, pDesc);

    // Query to external object if requested
    if (ppvResource) {
        hr = resource->QueryInterface(riidResource, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateCommittedResource2(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommittedResource2(table.next, pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state with lowered description
    resource = CreateResourceState(device, table, resource, reinterpret_cast<const D3D12_RESOURCE_DESC*>(pDesc));

    // Query to external object if requested
    if (ppvResource) {
        hr = resource->QueryInterface(riidResource, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateCommittedResource3(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riidResource, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommittedResource3(table.next, pHeapProperties, HeapFlags, pDesc, InitialLayout, pOptimizedClearValue, pProtectedSession, NumCastableFormats, pCastableFormats, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state with lowered description
    resource = CreateResourceState(device, table, resource, reinterpret_cast<const D3D12_RESOURCE_DESC*>(pDesc));

    // Query to external object if requested
    if (ppvResource) {
        hr = resource->QueryInterface(riidResource, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

template<typename T = D3D12_RESOURCE_DESC>
static ResourceCreateFlagSet GetPlacedResourceFlags(const T *desc) {
    // Creation flags
    ResourceCreateFlagSet flags = {};

    // If either a render target or depth stencil, this resource must be cleared
    // This is basically due to certain metadata, such as DCC/DeltaColorCompression, requiring
    // valid initial data.
    if ((desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
        flags |= ResourceCreateFlag::MetadataRequiresHardwareClear;
    }

    // OK
    return flags;
}

static D3D12_HEAP_FLAGS SanitizePlacedCommittedHeapFlags(D3D12_HEAP_FLAGS flags) {
    return flags & ~(
        D3D12_HEAP_FLAG_DENY_BUFFERS |
        D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES |
        D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS |
        D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES |
        D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES
    );
}

HRESULT HookID3D12DeviceCreatePlacedResource(ID3D12Device *device, ID3D12Heap * heap, UINT64 heapFlags, const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE * clearValue, const IID& riid, void ** pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Get the required flags
    ResourceCreateFlagSet flags = GetPlacedResourceFlags(desc);

#if USE_EMULATED_COMMITTED_ON_PLACED
    // TODO: Reshape should support generic modifications, so figure out a system for this
    // If we require a hardware clear, safe-guard the resource
    if (flags & ResourceCreateFlag::MetadataRequiresHardwareClear) {
        // Let the internal committed heap emulate the desired heap
        D3D12_HEAP_DESC heapDesc = heap->GetDesc();

        // Some flags are not appropriate for the emulated path
        heapDesc.Flags = SanitizePlacedCommittedHeapFlags(heapDesc.Flags);

        // Safe-guarded path
        HRESULT hr = table.bottom->next_CreateCommittedResource(table.next, &heapDesc.Properties, heapDesc.Flags, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
#endif // USE_EMULATED_COMMITTED_ON_PLACED

    // Application path
    if (!resource) {
        HRESULT hr = table.bottom->next_CreatePlacedResource(table.next, Next(heap), heapFlags, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create state
    resource = CreateResourceState(device, table, resource, desc, flags);

    // Query to external object if requested
    if (pResource) {
        HRESULT hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreatePlacedResource1(ID3D12Device* device, ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, const IID& riid, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Get the required flags
    ResourceCreateFlagSet flags = GetPlacedResourceFlags(pDesc);

#if USE_EMULATED_COMMITTED_ON_PLACED
    // TODO: Reshape should support generic modifications, so figure out a system for this
    // If we require a hardware clear, safe-guard the resource
    if (flags & ResourceCreateFlag::MetadataRequiresHardwareClear) {
        // Let the internal committed heap emulate the desired heap
        D3D12_HEAP_DESC heapDesc = pHeap->GetDesc();

        // Some flags are not appropriate for the emulated path
        heapDesc.Flags = SanitizePlacedCommittedHeapFlags(heapDesc.Flags);
        
        // Safe-guarded path
        HRESULT hr = table.bottom->next_CreateCommittedResource2(table.next, &heapDesc.Properties, heapDesc.Flags, pDesc, InitialState, pOptimizedClearValue, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
#endif // USE_EMULATED_COMMITTED_ON_PLACED

    // Application path
    if (!resource) {
        HRESULT hr = table.bottom->next_CreatePlacedResource1(table.next, Next(pHeap), HeapOffset, pDesc, InitialState, pOptimizedClearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create state with lowered description
    resource = CreateResourceState(device, table, resource, reinterpret_cast<const D3D12_RESOURCE_DESC*>(pDesc), flags);

    // Query to external object if requested
    if (ppvResource) {
        HRESULT hr = resource->QueryInterface(riid, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreatePlacedResource2(ID3D12Device* device, ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riid, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Get the required flags
    ResourceCreateFlagSet flags = GetPlacedResourceFlags(pDesc);

#if USE_EMULATED_COMMITTED_ON_PLACED
    // TODO: Reshape should support generic modifications, so figure out a system for this
    // If we require a hardware clear, safe-guard the resource
    if (flags & ResourceCreateFlag::MetadataRequiresHardwareClear) {
        // Let the internal committed heap emulate the desired heap
        D3D12_HEAP_DESC heapDesc = pHeap->GetDesc();

        // Some flags are not appropriate for the emulated path
        heapDesc.Flags = SanitizePlacedCommittedHeapFlags(heapDesc.Flags);
        
        // Safe-guarded path
        HRESULT hr = table.bottom->next_CreateCommittedResource3(table.next, &heapDesc.Properties, heapDesc.Flags, pDesc, InitialLayout, pOptimizedClearValue, nullptr, NumCastableFormats, pCastableFormats, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
    }
#endif // USE_EMULATED_COMMITTED_ON_PLACED

    // Application path
    if (!resource) {
        HRESULT hr = table.bottom->next_CreatePlacedResource2(table.next, Next(pHeap), HeapOffset, pDesc, InitialLayout, pOptimizedClearValue, NumCastableFormats, pCastableFormats, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create state with lowered description
    resource = CreateResourceState(device, table, resource, reinterpret_cast<const D3D12_RESOURCE_DESC*>(pDesc), flags);

    // Query to external object if requested
    if (ppvResource) {
        HRESULT hr = resource->QueryInterface(riid, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateReservedResource(ID3D12Device *device, const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE *clearValue, const IID& riid, void **pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateReservedResource(table.next, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(device, table, resource, desc, ResourceCreateFlag::Tiled);

    // Query to external object if requested
    if (pResource) {
        hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateReservedResource1(ID3D12Device* device, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riid, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateReservedResource1(table.next, pDesc, InitialState, pOptimizedClearValue, pProtectedSession, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(device, table, resource, pDesc, ResourceCreateFlag::Tiled);

    // Query to external object if requested
    if (ppvResource) {
        hr = resource->QueryInterface(riid, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateReservedResource2(ID3D12Device* device, const D3D12_RESOURCE_DESC* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riid, void** ppvResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateReservedResource2(table.next, pDesc, InitialLayout, pOptimizedClearValue,  pProtectedSession, NumCastableFormats, pCastableFormats, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(device, table, resource, pDesc, ResourceCreateFlag::Tiled);

    // Query to external object if requested
    if (ppvResource) {
        hr = resource->QueryInterface(riid, ppvResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

static void* CreateWrapperForSharedHandle(ID3D12Device* device, ID3D12Resource* resource) {
    auto table = GetTable(device);

    // Get the description
    D3D12_RESOURCE_DESC desc = resource->GetDesc();

    // Create a standard state
    return CreateResourceState(device, table, resource, &desc, ResourceCreateFlag::OpenedFromExternalHandle);
}

static void* CreateWrapperForSharedHandle(ID3D12Device* device, ID3D12Heap* heap) {
    auto table = GetTable(device);
    
    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) MemoryHeapState();
    state->allocators = table.state->allocators;
    state->parent = device;

    // Create detours
    return CreateDetour(state->allocators, heap, state);
}

static void* CreateWrapperForSharedHandle(ID3D12Device* device, ID3D12Fence* fence) {
    auto table = GetTable(device);
    
    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) FenceState();
    state->allocators = table.state->allocators;
    state->parent = device;

    // Create detours
    return CreateDetour(state->allocators, fence, state);
}

static void* CreateWrapperForSharedHandle(ID3D12Device* device, const IID& riid, void* object) {
    // Note: Can just check the uuids and cast to the same base, but this is _just_ a bit safer.
    
    // Resource handle?
    if (riid == __uuidof(ID3D12Resource2)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Resource2*>(object));
    } else if (riid == __uuidof(ID3D12Resource1)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Resource1*>(object));
    } else if (riid == __uuidof(ID3D12Resource)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Resource*>(object));
    }

    // Heap handle?
    else if (riid == __uuidof(ID3D12Heap1)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Heap1*>(object));
    } else if (riid == __uuidof(ID3D12Heap)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Heap*>(object));
    }

    // Fence handle?
    else if (riid == __uuidof(ID3D12Fence1)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Fence1*>(object));
    } else if (riid == __uuidof(ID3D12Fence)) {
        return CreateWrapperForSharedHandle(device, static_cast<ID3D12Fence*>(object));
    }

    // Shouldn't get here
    ASSERT(false, "Invalid shared handle UUID");
    return nullptr;
}

HRESULT WINAPI HookID3D12DeviceOpenSharedHandle(ID3D12Device* device, HANDLE NTHandle, const IID& riid, void** ppvObj) {
    auto table = GetTable(device);

    // Bottom object
    void* object;

    // Pass down call chain
    HRESULT hr = table.next->OpenSharedHandle(NTHandle, riid, &object);
    if (FAILED(hr)) {
        return hr;
    }

    // Create the wrapper
    *ppvObj = CreateWrapperForSharedHandle(device, riid, object);

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12ResourceGetDevice(ID3D12Resource* _this, REFIID riid, void **ppDevie) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevie);
}

HRESULT WINAPI HookID3D12ResourceSetName(ID3D12Resource* _this, LPCWSTR name) {
    auto table = GetTable(_this);

    // Get device
    auto deviceTable = GetTable(table.state->parent);

    // Get length
    size_t length;
    ENSURE(SUCCEEDED(wcstombs_s(&length, nullptr, 0u, name, 0u)), "Failed to determine length");

    // Copy string
    char* debugName = new (table.state->allocators) char[length];
    ENSURE(SUCCEEDED(wcstombs_s(&length, debugName, length, name, length)), "Failed to convert string");

    // Inform controller of the change
    deviceTable.state->versioningController->CreateOrRecommitResource(table.state, debugName);

    // Serialize all naming assignment
    // Could alternatively serialize per-object, however, that's a lot of synchronization primitives
    {
        std::lock_guard guard(deviceTable.state->states_Resources.GetLock());

        // Release previous name
        if (table.state->debugName) {
            destroy(table.state->debugName, table.state->allocators);
        }

        // Assign new name
        table.state->debugName = debugName;
    }
    
    // Pass to device query
    return table.state->parent->SetName(name);
}

HRESULT WINAPI HookID3D12DeviceSetResidencyPriority(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects, const D3D12_RESIDENCY_PRIORITY* pPriorities){
    auto table = GetTable(_this);
    
    // Unwrap all objects
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Pageable*, NumObjects);
    for (uint32_t i = 0; i < NumObjects; i++) {
        unwrapped[i] = UnwrapObject(ppObjects[i]);
    }

    // Pass down callchain
    return table.next->SetResidencyPriority(NumObjects, unwrapped, pPriorities);
}

HRESULT WINAPI HookID3D12DeviceMakeResident(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects){
    auto table = GetTable(_this);
    
    // Unwrap all objects
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Pageable*, NumObjects);
    for (uint32_t i = 0; i < NumObjects; i++) {
        unwrapped[i] = UnwrapObject(ppObjects[i]);
    }

    // Pass down callchain
    return table.next->MakeResident(NumObjects, unwrapped);
}

HRESULT WINAPI HookID3D12DeviceEnqueueMakeResident(ID3D12Device* _this, D3D12_RESIDENCY_FLAGS Flags, UINT NumObjects, ID3D12Pageable* const* ppObjects, ID3D12Fence* pFenceToSignal, UINT64 FenceValueToSignal){
    auto table = GetTable(_this);
    
    // Unwrap all objects
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Pageable*, NumObjects);
    for (uint32_t i = 0; i < NumObjects; i++) {
        unwrapped[i] = UnwrapObject(ppObjects[i]);
    }

    // Pass down callchain
    return table.next->EnqueueMakeResident(Flags, NumObjects, unwrapped, Next(pFenceToSignal), FenceValueToSignal);
}

HRESULT WINAPI HookID3D12DeviceEvict(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects) {
    auto table = GetTable(_this);
    
    // Unwrap all objects
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Pageable*, NumObjects);
    for (uint32_t i = 0; i < NumObjects; i++) {
        unwrapped[i] = UnwrapObject(ppObjects[i]);
    }

    // Pass down callchain
    return table.next->Evict(NumObjects, unwrapped);
}

ResourceState::~ResourceState() {
    auto table = GetTable(parent);

    // May be invalid
    if (!object) {
        parent->Release();
        return;
    }
    
    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table.state->featureHookTables) {
        proxyTable.destroyResource.TryInvoke(GetResourceInfoFor(this));
    }

    // Release name
    if (debugName) {
        destroy(debugName, allocators);
    }

    // Remove mapping
    switch (desc.Dimension) {
        default:
            break;
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            table.state->virtualAddressTable.Remove(object->GetGPUVirtualAddress());
            break;
    }

    // Inform controller
    table.state->versioningController->DestroyResource(this);

    // Remove state
    table.state->states_Resources.Remove(this);

    // Release parent
    parent->Release();
}
