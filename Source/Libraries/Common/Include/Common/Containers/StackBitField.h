#pragma once

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Stack bit field, bit field with default stack container, and optional heap fallback
struct StackBitField {
    /// Initialize with size
    StackBitField(size_t count) {
        bitfield.Resize((count + 32 - 1) / 32);
        Clear();
    }

    /// Clear all bits
    void Clear() {
        std::memset(bitfield.Data(), 0x0u, bitfield.Size() * sizeof(uint32_t));
    }

    /// Set a bit
    void Set(uint32_t i) {
        GetElement(i) |= 1u << (i % 32);
    }

    /// Clear a bit
    void Clear(uint32_t i) {
        GetElement(i) &= ~(1u << (i % 32));
    }

    /// Get a bit state
    bool Get(uint32_t i) const {
        return GetElement(i) & (1u << (i % 32));
    }

private:
    /// Get the element for a given bit
    uint32_t& GetElement(uint32_t i) {
        return bitfield[i / 32];
    }

    /// Get the element for a given bit
    uint32_t GetElement(uint32_t i) const {
        return bitfield[i / 32];
    }

private:
    TrivialStackVector<uint32_t, 128> bitfield;
};