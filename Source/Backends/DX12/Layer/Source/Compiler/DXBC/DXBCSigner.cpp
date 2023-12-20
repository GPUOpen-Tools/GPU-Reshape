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

#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>

// Common
#include <Common/FileSystem.h>

// System
#include <Windows.h>

// Std
#include <string>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

bool DXBCSigner::Install() {
#if USE_DXBC_SIGNER
    // Get path of the layer
    std::filesystem::path modulePath = GetBaseModuleDirectory();

    // Load DXBC
    //   ! No non-system/runtime dependents in dxbcsigner.dll, verified with dumpbin
    module = LoadLibrary((modulePath / "dxbcsigner.dll").string().c_str());
    if (!module) {
        return false;
    }

    // Get gpa
    dxbcSign = reinterpret_cast<PFN_DXBCSign>(GetProcAddress(module, "SignDxbc"));
#endif // USE_DXBC_SIGNER

    // OK
    return true;
}

DXBCSigner::~DXBCSigner() {
#if USE_DXBC_SIGNER
    if (module) {
        FreeLibrary(module);
    }
#endif // USE_DXBC_SIGNER
}

bool DXBCSigner::Sign(void *code, uint64_t length) {
#if USE_DXBC_SIGNER
    return SUCCEEDED(dxbcSign(static_cast<BYTE*>(code), static_cast<uint32_t>(length)));
#else // USE_DXBC_SIGNER
    return false;
#endif // USE_DXBC_SIGNER
}
