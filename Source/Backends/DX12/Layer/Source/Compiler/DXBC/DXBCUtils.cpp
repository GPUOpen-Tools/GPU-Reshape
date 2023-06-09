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
