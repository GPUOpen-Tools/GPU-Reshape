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

#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/Layer.h>

// System
#include <Windows.h>

// Common
#include <Common/FileSystem.h>

// Std
#include <string>

// MD5
#include <MD5/MD5.h>

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
#if USE_DXIL_BYPASS_SIGNING
    // If using the signing bypass, the method is fast enough as is
    // also, this avoids issues with some Agility SDKs requiring signing
    // regardless.
    needsSigning = true;
#else // USE_DXIL_BYPASS_SIGNING
    // Experimental shading models bypass signers
    needsSigning = !D3D12GPUOpenProcessInfo.isExperimentalShaderModelsEnabled;
#endif // USE_DXIL_BYPASS_SIGNING

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

    // Use bypass signing?
#if USE_DXIL_BYPASS_SIGNING
    // If in debug, try to actually validate and sign it.
    // This may produce some false positives if the source binaries
    // are invalid, but they can be ignored.
#if !defined(NDEBUG)
    if (SignWithValidation(code, length)) {
        return true;
    }
#endif // !NDEBUG

    // Just trust the instrumented binaries
    SignWithBypass(code);
    return true;
#else // USE_DXIL_BYPASS_SIGNING
    // No bypass, perform full validation and signing
    return SignWithValidation(code, length);
#endif // USE_DXIL_BYPASS_SIGNING
}

bool DXILSigner::SignWithValidation(void *code, uint64_t length) {
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

bool DXILSigner::SignWithBypass(void *code) {
    auto header = static_cast<DXBCHeader*>(code);

    // Create context
    MD5_CTX ctx{};
    MD5_Init(&ctx);
    
    // Chunk attributes
    uint32_t byteCount        = static_cast<uint32_t>(header->byteCount - offsetof(DXBCHeader, reserved));
    uint32_t bitCount         = byteCount * 8u;
    uint32_t lastChunkLength  = byteCount % 64;
    uint32_t lastChunkPadding = 64 - lastChunkLength;
    uint32_t fullChunkLength  = byteCount - lastChunkLength;
    uint32_t bitCount2o1      = (bitCount >> 2u) | 1u;

    // Update all full chunks
    MD5_Update(&ctx, &header->reserved, fullChunkLength);

    // Last chunk address
    auto* danglingChunk = reinterpret_cast<const uint8_t*>(&header->reserved) + fullChunkLength;

    // Final block
    uint32_t md5Block[16]{};
    md5Block[0] = 0x80;

    // Magic DXIL signing check
    if (lastChunkLength >= 56) {
        // Update last block
        MD5_Update(&ctx, danglingChunk, lastChunkLength);

        // Update padding block
        MD5_Update(&ctx, md5Block, lastChunkPadding);

        // Update final block
        md5Block[0]  = bitCount;
        md5Block[15] = bitCount2o1;
        MD5_Update(&ctx, md5Block, 64);
    } else {
        // Update low number of bits
        MD5_Update(&ctx, &bitCount, sizeof(bitCount));

        // Update last block if needed
        if (lastChunkLength) {
            MD5_Update(&ctx, danglingChunk, lastChunkLength);
        }

        // Write remainder length
        uint32_t paddingBytes = static_cast<uint32_t>(lastChunkPadding - sizeof(uint32_t));
        std::memcpy(reinterpret_cast<uint8_t*>(md5Block) + paddingBytes - sizeof(uint32_t), &bitCount2o1, sizeof(uint32_t));

        // Update final block
        MD5_Update(&ctx, md5Block, paddingBytes);
    }

    // Copy over final checksum
    static_assert(sizeof(header->privateChecksum) == sizeof(ctx.a) * 4u, "Unexpected checksum size");
    std::memcpy(&header->privateChecksum, &ctx.a, sizeof(header->privateChecksum));

    // Always ok
    return true;
}
