#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>

// Std
#include <string>

// System
#include <Windows.h>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

/*
 * Partially based on https://github.com/gwihlidal/dxil-signing, thank you Graham.
 * */

bool DXILSigner::Install() {
    // Load dxil
    dxilModule = LoadLibrary("dxil.dll");
    if (!dxilModule) {
        return false;
    }

    // Load dxcompiler
    dxCompilerModule = LoadLibrary("dxcompiler.dll");
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

    // OK
    return true;
}

DXILSigner::~DXILSigner() {
    if (dxilModule) {
        FreeLibrary(dxilModule);
    }

    if (dxCompilerModule) {
        FreeLibrary(dxCompilerModule);
    }
}

bool DXILSigner::Sign(void *code, uint64_t length) {
    Microsoft::WRL::ComPtr<IDxcOperationResult> result;

    // Create a pinned blob (no-copy)
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> pinnedBlob;
    library->CreateBlobWithEncodingFromPinned(static_cast<BYTE*>(code), length, 0u, pinnedBlob.GetAddressOf());

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
