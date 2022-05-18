#pragma once

// Std
#include <cstdint>

/// DXBC parsing context
struct DXBCParseContext {
    DXBCParseContext(const void* ptr, uint32_t length) :
        start(reinterpret_cast<const uint8_t*>(ptr)),
        ptr(reinterpret_cast<const uint8_t*>(ptr)),
        end(reinterpret_cast<const uint8_t*>(ptr) + length) {
        /* */
    }

    /// Get the current offset as a type
    template<typename T>
    const T &Get() const {
        return *reinterpret_cast<const T *>(ptr);
    }

    /// Get the current offset as a type and consume the respective bytes
    template<typename T>
    const T &Consume() {
        const T *value = reinterpret_cast<const T *>(ptr);
        ptr += sizeof(T);
        return *value;
    }

    /// Get the absolute offset as a type
    template<typename T>
    const T* ReadAt(uint32_t offset) {
        return reinterpret_cast<const T*>(start + offset);
    }

    /// Get the current offset as a type
    template<typename T>
    const T* ReadAtOffset(uint32_t offset) {
        return reinterpret_cast<const T*>(ptr + offset);
    }

    /// Is the stream in a good state?
    bool IsGood() const {
        return ptr < end;
    }

    /// Can we parse the next N bytes?
    bool IsGoodFor(uint32_t size) const {
        return ptr + size <= end;
    }

    const uint8_t *start;
    const uint8_t *ptr;
    const uint8_t *end;
};
