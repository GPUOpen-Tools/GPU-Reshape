#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreatePipelineLibrary(ID3D12Device* device, const void *pLibraryBlob, SIZE_T BlobLength, REFIID riid, void **ppPipelineLibrary);
HRESULT WINAPI HookID3D12PipelineLibraryStorePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, ID3D12PipelineState *pPipeline);
HRESULT WINAPI HookID3D12PipelineLibraryLoadGraphicsPipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState);
HRESULT WINAPI HookID3D12PipelineLibraryLoadComputePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState);
HRESULT WINAPI HookID3D12PipelineLibraryGetDevice(ID3D12PipelineLibrary* _this, REFIID riid, void **ppDevice);
