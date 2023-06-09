#include <Common/CRC.h>

// ZLIB
#include <zlib.h>

uint32_t BufferCRC32LongStart() {
    return crc32(0u, nullptr, 0);
}

uint32_t BufferCRC32Long(const void* data, uint32_t len, uint32_t crc) {
    return crc32(crc, static_cast<const Bytef*>(data), len);
}
