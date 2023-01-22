#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>

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
