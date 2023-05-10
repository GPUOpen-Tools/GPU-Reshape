#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE* clearValue, const IID& riid, void** pResource);
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource1(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource2(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreatePlacedResource(ID3D12Device*, ID3D12Heap*, UINT64, const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreatePlacedResource1(ID3D12Device*, ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, const IID& riid, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateReservedResource(ID3D12Device*, const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, const IID&, void**);
HRESULT WINAPI HookID3D12ResourceMap(ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* readRange, void** blob);
HRESULT WINAPI HookID3D12ResourceGetDevice(ID3D12Resource* _this, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12ResourceSetName(ID3D12Resource* _this, LPCWSTR name);
