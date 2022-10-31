#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateDescriptorHeap(ID3D12Device*, const D3D12_DESCRIPTOR_HEAP_DESC*, REFIID, void **);
void WINAPI HookID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap*, D3D12_CPU_DESCRIPTOR_HANDLE*);
void WINAPI HookID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap*, D3D12_GPU_DESCRIPTOR_HANDLE*);
void WINAPI HookID3D12DeviceCreateShaderResourceView(ID3D12Device* _this, ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);
void WINAPI HookID3D12DeviceCreateUnorderedAccessView(ID3D12Device* _this, ID3D12Resource* pResource, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);
HRESULT WINAPI HookID3D12DescriptorHeapGetDevice(ID3D12DescriptorHeap* _this, REFIID riid, void **ppDevice);
