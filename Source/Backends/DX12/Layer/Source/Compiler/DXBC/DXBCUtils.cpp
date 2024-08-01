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

#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXBC/DXBCUtils.h>
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockType.h>

// Common
#include <Common/CRC.h>

// Std
#include <algorithm>

bool ScanDXBCShaderDigest(const void* byteCode, uint64_t byteLength, DXILDigest& digest) {
    DXBCParseContext ctx(byteCode, byteLength);
    
    // Consume header
    auto header = ctx.Consume<DXBCHeader>();
    
    // Must be DXBC
    if (header.identifier != 'CBXD') {
        return false;
    }

    // Handle all chunks
    for (uint32_t chunkIndex = 0; chunkIndex < header.chunkCount; chunkIndex++) {
        auto chunk = ctx.Consume<DXBCChunkEntryHeader>();

        // Header of the chunk
        auto chunkHeader = ctx.ReadAt<DXBCChunkHeader>(chunk.offset);

        // Is hash?
        if (chunkHeader->type != static_cast<uint32_t>(DXBCPhysicalBlockType::ShaderHash)) {
            continue;
        }

        // Copy digest
        const auto* shaderHash = ctx.ReadAt<DXILShaderHash>(chunk.offset + sizeof(DXBCChunkHeader));
        std::memcpy(&digest, &shaderHash->digest, sizeof(digest));

        // OK
        return true;
    }

    // No matching block
    return false;
}

uint32_t GetOrComputeDXBCShaderHash(const void* byteCode, uint64_t byteLength) {
    // Check if the blob has a digest
    DXILDigest digest;
    if (ScanDXBCShaderDigest(byteCode, byteLength, digest)) {
        return BufferCRC32Short(&digest, sizeof(digest));
    }

    // Initial crc
    uint32_t crc = BufferCRC32LongStart();

    // Compute long hash
    for (size_t remaining = byteLength, offset = 0; offset < byteLength; offset += ~0u) {
        crc = BufferCRC32Long(
            static_cast<const uint8_t*>(byteCode) + offset,
            static_cast<uint32_t>(std::min<uint64_t>(~0u, remaining - offset)),
            crc
        );
    }

    // OK
    return crc;
}

bool IsDXBCNative(const void *byteCode, uint64_t byteLength) {
    DXBCParseContext ctx(byteCode, byteLength);
    
    // Consume header
    auto header = ctx.Consume<DXBCHeader>();
    
    // Must be DXBC
    if (header.identifier != 'CBXD') {
        return false;
    }

    // Handle all chunks
    for (uint32_t chunkIndex = 0; chunkIndex < header.chunkCount; chunkIndex++) {
        auto chunk = ctx.Consume<DXBCChunkEntryHeader>();

        // Is DXIL?
        if (ctx.ReadAt<DXBCChunkHeader>(chunk.offset)->type == static_cast<uint32_t>(DXBCPhysicalBlockType::DXIL)) {
            return false;
        }
    }

    // No DXIL data found, native
    return true;
}

static char PartAsHex(int32_t part) {
    if (part < 10) {
        return static_cast<char>('0' + part);
    } else {
        return static_cast<char>('a' + part - 10);
    }
}

void DXBCShaderDigestToString(const DXILDigest& digest, char buffer[kDXBCShaderDigestStringLength]) {
    static_assert(sizeof(digest.digest) * 2u == kDXBCShaderDigestStringLength - 1, "Unexpected digest size");
    
    // Print each 4-bit window
    for (uint32_t i = 0; i < std::size(digest.digest); i++) {
        buffer[i * 2 + 0] = PartAsHex(digest.digest[i] >> 0x4);
        buffer[i * 2 + 1] = PartAsHex(digest.digest[i]  & 0xF);
    }

    // Null terminator
    buffer[32] = '\0';
}
