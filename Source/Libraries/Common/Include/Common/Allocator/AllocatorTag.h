#pragma once

// Common
#include <Common/CRC.h>
#include <Common/Common.h>

/// Identifiable allocation tag
struct AllocationTag {
    constexpr AllocationTag(uint64_t crc64 = 0ull, const char* name = nullptr) : crc64(crc64), name(name) {
        
    }

    /// Hash data
    uint64_t crc64;

    /// Windowed name
    const char* name;
};

/// Tag literal
GRS_CONSTEVAL AllocationTag operator "" _AllocTag(const char* data, size_t len) {
    return AllocationTag(crcdetail::compute(data, len), data);
}

/// Default tag
static constexpr AllocationTag kDefaultAllocTag = AllocationTag(0u, "Default");
