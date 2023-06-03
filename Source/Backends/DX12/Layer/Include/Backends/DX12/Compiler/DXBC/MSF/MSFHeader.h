#pragma once

// Std
#include <cstdint>

static constexpr char kMSFSuperBlockMagic[] = {'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', ' ', 'C', '/', 'C', '+', '+', ' ', 'M', 'S', 'F', ' ', '7', '.', '0', '0', '\r', '\n', 0x1A, 0x44, 0x53, 0x0, 0x0, 0x0};

struct MSFSuperBlock {
    char magic[sizeof(kMSFSuperBlockMagic)];
    uint32_t blockSize;
    uint32_t freeBlockMapBlock;
    uint32_t blockCount;
    uint32_t directoryByteCount;
    uint32_t unknown;
    uint32_t blockMapAddr;
};
