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

#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Layer.h>

// System
#include <Windows.h>

// Common
#include <Common/FileSystem.h>

// Std
#include <string>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

/*
 * Partially based on https://github.com/gwihlidal/dxil-signing, thank you Graham.
 * */

bool DXILSigner::Install() {
    // Get path of the layer
    std::filesystem::path modulePath = GetBaseModuleDirectory();

    // Load dxil
    //   ! No non-system/runtime dependents in dxil.dll, verified with dumpbin
    dxilModule = LoadLibrary((modulePath / "dxil.dll").string().c_str());
    if (!dxilModule) {
        return false;
    }

    // Load dxcompiler
    //   ! No non-system/runtime dependents in dxcompiler.dll, verified with dumpbin
    dxCompilerModule = LoadLibrary((modulePath / "dxcompiler.dll").string().c_str());
    if (!dxCompilerModule) {
        return false;
    }

    // Get gpa's
    auto gpaDXILCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxilModule, "DxcCreateInstance"));
    auto gpaDXCCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxCompilerModule, "DxcCreateInstance"));

    // Missing?
    if (!gpaDXILCreateInstance || !gpaDXCCreateInstance) {
        return false;
    }

    // Try to create a library instance
    if (FAILED(gpaDXCCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&library))) {
        return false;
    }

    // Try to create a validator instance
    if (FAILED(gpaDXILCreateInstance(CLSID_DxcValidator, __uuidof(IDxcValidator), (void**)&validator))) {
        return false;
    }

    // Signing may not be required
    needsSigning = !D3D12GPUOpenProcessInfo.isExperimentalShaderModelsEnabled;

    // OK
    return true;
}

DXILSigner::~DXILSigner() {
    validator.Reset();
    library.Reset();

    if (dxilModule) {
        FreeLibrary(dxilModule);
    }

    if (dxCompilerModule) {
        FreeLibrary(dxCompilerModule);
    }
}

bool DXILSigner::Sign(void *code, uint64_t length) {
    Microsoft::WRL::ComPtr<IDxcOperationResult> result;

    // Early out if not needed
    //  ! Keep it in for debug validation
#if defined(NDEBUG)
    if (!needsSigning) {
        return true;
    }
#endif // defined(NDEBUG)

    // Create a pinned blob (no-copy)
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> pinnedBlob;
    library->CreateBlobWithEncodingFromPinned(static_cast<BYTE*>(code), static_cast<uint32_t>(length), 0u, pinnedBlob.GetAddressOf());

    // Attempt to sign the code (no-copy)
    if (FAILED(validator->Validate(pinnedBlob.Get(), DxcValidatorFlags_InPlaceEdit, &result))) {
        return false;
    }

    // Get status
    HRESULT status;
    if (FAILED(result->GetStatus(&status))) {
        return false;
    }

    // Did we fail?
    if (FAILED(status)) {
#ifndef NDEBUG
        // Get error
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBuffer;
        result->GetErrorBuffer(&errorBuffer);

        // To UTF8
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBufferUTF8;
        library->GetBlobAsUtf8(errorBuffer.Get(), errorBufferUTF8.GetAddressOf());

        // Compose error
        std::stringstream ss;
        ss << "DXIL Signing failed: " << static_cast<const char*>(errorBufferUTF8->GetBufferPointer());
        ASSERT(false, ss.str().c_str());
#endif // NDEBUG

        return false;
    }

    // OK
    return true;
}
