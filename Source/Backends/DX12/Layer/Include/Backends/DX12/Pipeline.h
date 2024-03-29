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

#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Forward declarations
struct ShaderState;
struct DeviceState;
struct PipelineSubObjectWriter;

/// Get an existing shader state or create a new one
/// \param device the parent device
/// \param byteCode user shader byte code 
ShaderState *GetOrCreateShaderState(DeviceState *device, const D3D12_SHADER_BYTECODE &byteCode);

/// Unwrap a pipeline state stream
/// \param writer the destination writer
/// \param desc the wrapped pipeline state stream 
/// \param rootSignature the optional root signature
D3D12_PIPELINE_STATE_STREAM_DESC UnwrapPipelineStateStream(PipelineSubObjectWriter& writer, const D3D12_PIPELINE_STATE_STREAM_DESC* desc, ID3D12RootSignature** rootSignature);

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreatePipelineState(ID3D12Device2*, const D3D12_PIPELINE_STATE_STREAM_DESC*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateStateObject(ID3D12Device2*, const D3D12_STATE_OBJECT_DESC* pDesc, const IID& riid, void** ppStateObject);
HRESULT WINAPI HookID3D12DeviceAddToStateObject(ID3D12Device2*, const D3D12_STATE_OBJECT_DESC* pAddition, ID3D12StateObject* pStateObjectToGrowFrom, const IID& riid, void** ppNewStateObject);
HRESULT WINAPI HookID3D12StateObjectGetDevice(ID3D12StateObject*, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12DeviceCreateGraphicsPipelineState(ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateComputePipelineState(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, const IID&, void**);
HRESULT WINAPI HookID3D12PipelineStateGetDevice(ID3D12PipelineState* _this, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12PipelineStateSetName(ID3D12PipelineState* _this, LPCWSTR name);
