#pragma once

// Std
#include <cstdint>

/// Shader header
struct DXBCHeader {
    uint32_t identifier;
    uint8_t privateChecksum[16];
    uint32_t reserved;
    uint32_t byteCount;
    uint32_t chunkCount;
};

/// Post header chunk entry header
struct DXBCChunkEntryHeader {
    uint32_t offset;
};

/// Chunk header at offset specified
struct DXBCChunkHeader {
    uint32_t type;
    uint32_t size;
};

/// Shader header
struct DXBCShaderHeader : public DXBCChunkHeader {
    uint8_t minor : 4;
    uint8_t major : 4;
    uint8_t type;
    uint32_t dwordCount;
};
