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

#include <Backends/DX12/Compiler/DXBC/DXBCConverter.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Registry.h>

// System
#include <Windows.h>

// Std
#include <string>

// DXC
#include <DXC/dxcapi.h>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

bool DXBCConverter::Install() {
    signer = registry->Get<DXILSigner>();
    if (!signer) {
        return false;
    }
    
    // Get path of the layer
    std::filesystem::path modulePath = GetBaseModuleDirectory();

    // Load dxil
    //   ! No non-system/runtime dependents in dxilconv.dll, verified with dumpbin
    dxilConvModule = LoadLibrary((modulePath / "dxilconv.dll").string().c_str());
    if (!dxilConvModule) {
        return false;
    }

    // Get gpa's
    auto gpaDXILCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxilConvModule, "DxcCreateInstance"));

    // Missing?
    if (!gpaDXILCreateInstance) {
        return false;
    }

    // Try to create a library instance
    if (FAILED(gpaDXILCreateInstance(CLSID_DxbcConverter, __uuidof(IDxbcConverter), (void**)&converter))) {
        return false;
    }

    // OK
    return true;
}

DXBCConverter::~DXBCConverter() {
    converter.Reset();

    if (dxilConvModule) {
        FreeLibrary(dxilConvModule);
    }
}

bool DXBCConverter::Convert(const void* code, uint64_t length, DXBCConversionBlob* out) {
    // Note: Seeming instability in threaded environments...
    std::lock_guard guard(mutex);

    // Convert from DXBC -> DXIL
    if (FAILED(converter->Convert(
        code, static_cast<uint32_t>(length),
        nullptr,
        reinterpret_cast<void**>(&out->blob),
        &out->length,
        nullptr
    ))) {
        return false;
    }

    // Interestingly the resulting DXIL is not sign-proof. I assume that it's safe for driver consumption, as it's internally "signed",
    // however, this means that we cannot sign instrumented shaders, which is a pity.
#if 0
    if (!signer->Sign(out->blob, out->length, false)) {
        ASSERT(false, "Failed to sign converted DXBC");
        return false;
    }
#endif // NDEBUG

    return true;
}
