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

// Forward declarations
struct DeviceState;
struct RootRegisterBindingInfo;
struct RootSignatureLogicalMapping;
struct RootSignaturePhysicalMapping;

/// Serialize a root signature for instrumentation
/// \param state device state
/// \param version signature version
/// \param source signature data
/// \param out serialized blob
/// \param outRoot root bindings
/// \param outLogical logical physical mappings
/// \param outMapping root physical mappings
/// \param outError optional, error blob
/// \return result
template<typename T>
HRESULT SerializeRootSignature(DeviceState* state, D3D_ROOT_SIGNATURE_VERSION version, const T& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, RootSignatureLogicalMapping* outLogical, RootSignaturePhysicalMapping** outMapping, ID3DBlob** outError);

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateRootSignature(ID3D12Device*, UINT, const void*, SIZE_T, const IID&, void**);
HRESULT WINAPI HookID3D12RootSignatureGetDevice(ID3D12RootSignature* _this, REFIID riid, void **ppDevice);
