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
#include <Backends/DX12/Layer.h>

/// Hooks
DX12_C_LINKAGE HRESULT WINAPI HookD3D11On12CreateDevice(IUnknown *pDevice, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, IUnknown *const *ppCommandQueues, UINT NumQueues, UINT NodeMask, ID3D11Device **ppDevice, ID3D11DeviceContext **ppImmediateContext, D3D_FEATURE_LEVEL *pChosenFeatureLevel);
HRESULT WINAPI HookID3D11DeviceQueryInterface(ID3D11Device* device, const IID& riid, void** ppvObject);
void WINAPI HookID3D11On12DeviceReleaseWrappedResources(ID3D11On12Device* device, ID3D11Resource* const* ppResources, UINT NumResources);
void WINAPI HookID3D11On12DeviceAcquireWrappedResources(ID3D11On12Device* device, ID3D11Resource* const* ppResources, UINT NumResources);
HRESULT WINAPI HookID3D11On12DeviceReturnUnderlyingResource(ID3D11On12Device* device, ID3D11Resource* pResource11, UINT NumSync, UINT64* pSignalValues, ID3D12Fence** ppFences);
