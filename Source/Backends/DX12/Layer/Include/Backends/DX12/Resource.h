// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE* clearValue, const IID& riid, void** pResource);
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource1(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource2(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialResourceState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riidResource, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateCommittedResource3(ID3D12Device* _this, const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riidResource, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreatePlacedResource(ID3D12Device*, ID3D12Heap*, UINT64, const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreatePlacedResource1(ID3D12Device*, ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, const IID& riid, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreatePlacedResource2(ID3D12Device* _this, ID3D12Heap* pHeap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC1* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riid, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateReservedResource(ID3D12Device*, const D3D12_RESOURCE_DESC*, D3D12_RESOURCE_STATES, const D3D12_CLEAR_VALUE*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateReservedResource1(ID3D12Device*,const D3D12_RESOURCE_DESC* pDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, const IID& riid, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceCreateReservedResource2(ID3D12Device* _this, const D3D12_RESOURCE_DESC* pDesc, D3D12_BARRIER_LAYOUT InitialLayout, const D3D12_CLEAR_VALUE* pOptimizedClearValue, ID3D12ProtectedResourceSession* pProtectedSession, UINT32 NumCastableFormats, const DXGI_FORMAT* pCastableFormats, const IID& riid, void** ppvResource);
HRESULT WINAPI HookID3D12DeviceOpenSharedHandle(ID3D12Device*, HANDLE NTHandle, const IID& riid, void** ppvObj);
HRESULT WINAPI HookID3D12DeviceOpenSharedHandleByName(ID3D12Device*, LPCWSTR Name, DWORD Access, HANDLE* pNTHandle);
HRESULT WINAPI HookID3D12ResourceMap(ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* readRange, void** blob);
HRESULT WINAPI HookID3D12ResourceGetDevice(ID3D12Resource* _this, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12ResourceSetName(ID3D12Resource* _this, LPCWSTR name);
HRESULT WINAPI HookID3D12DeviceSetResidencyPriority(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects, const D3D12_RESIDENCY_PRIORITY* pPriorities);
HRESULT WINAPI HookID3D12DeviceMakeResident(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects);
HRESULT WINAPI HookID3D12DeviceEnqueueMakeResident(ID3D12Device* _this, D3D12_RESIDENCY_FLAGS Flags, UINT NumObjects, ID3D12Pageable* const* ppObjects, ID3D12Fence* pFenceToSignal, UINT64 FenceValueToSignal);
HRESULT WINAPI HookID3D12DeviceEvict(ID3D12Device* _this, UINT NumObjects, ID3D12Pageable* const* ppObjects);
