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

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Compiler/DXBC/DXBCConversionBlob.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// System
#include <wrl/client.h>

// DXC
#include <DXC/dxcapi.h>
#include <DXC/DxbcConverter.h>

// Std
#include <mutex>

// Forward declarations
class DXILSigner;

/// Function types
using PFN_DXBCSign = HRESULT(*)(BYTE* pData, UINT32 byteCount);

class DXBCConverter : public TComponent<DXBCConverter> {
public:
    COMPONENT(DXBCConverter);

    /// Deconstructor
    ~DXBCConverter();

    /// Install this signer
    /// \return false if failed
    bool Install();

    /// Convert a given DXBC blob to DXIL
    /// \param code DXBC blob start
    /// \param length DXBC blob byte length
    /// \param out destination DXIL blob
    /// \return success state
    bool Convert(const void* code, uint64_t length, DXBCConversionBlob* out);

private:
    /// Objects
    Microsoft::WRL::ComPtr<IDxbcConverter> converter;

    /// Components
    ComRef<DXILSigner> signer;

    /// Shared lock
    std::mutex mutex;

private:
    /// Dynamic modules
    HMODULE dxilConvModule{nullptr};

private:
    /// Dynamic module
    HMODULE module{nullptr};
};
