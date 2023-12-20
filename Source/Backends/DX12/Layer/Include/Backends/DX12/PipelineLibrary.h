// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreatePipelineLibrary(ID3D12Device* device, const void *pLibraryBlob, SIZE_T BlobLength, REFIID riid, void **ppPipelineLibrary);
HRESULT WINAPI HookID3D12PipelineLibraryStorePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, ID3D12PipelineState *pPipeline);
HRESULT WINAPI HookID3D12PipelineLibraryLoadGraphicsPipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState);
HRESULT WINAPI HookID3D12PipelineLibraryLoadComputePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState);
HRESULT WINAPI HookID3D12PipelineLibraryLoadPipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc, const IID& riid, void** ppPipelineState);
HRESULT WINAPI HookID3D12PipelineLibraryGetDevice(ID3D12PipelineLibrary* _this, REFIID riid, void **ppDevice);
