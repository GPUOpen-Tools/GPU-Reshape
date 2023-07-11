// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Common
#include <Common/IComponent.h>

/// Function types
using PFN_DXBCSign = HRESULT(*)(BYTE* pData, UINT32 byteCount);

class DXBCSigner : public TComponent<DXBCSigner> {
public:
    COMPONENT(DXBCSigner);

    /// Deconstructor
    ~DXBCSigner();

    /// Install this signer
    /// \return false if failed
    bool Install();

    /// Convert a DXBC blob
    /// \param code blob code
    /// \param length blob length
    /// \return false if failed
    bool Sign(void* code, uint64_t length);

private:
    /// Proc
    PFN_DXBCSign dxbcSign{};

private:
    /// Dynamic module
    HMODULE module{nullptr};
};
