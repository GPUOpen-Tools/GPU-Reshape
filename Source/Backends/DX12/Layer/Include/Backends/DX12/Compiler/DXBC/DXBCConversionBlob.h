#pragma once

// Std
#include <cstdint>

struct DXBCConversionBlob {
    /// Constructor
    DXBCConversionBlob() = default;

    /// Destructor
    ~DXBCConversionBlob() {
        delete blob;
    }

    /// No copy
    DXBCConversionBlob(const DXBCConversionBlob&) = delete;
    DXBCConversionBlob operator=(const DXBCConversionBlob&) = delete;

    /// Move constructor
    DXBCConversionBlob(DXBCConversionBlob&& other) noexcept : blob(other.blob), length(other.length) {
        other.blob = nullptr;
        other.length = 0;
    }

    /// Move assignment
    DXBCConversionBlob& operator=(DXBCConversionBlob&& other) noexcept {
        blob = other.blob;
        length = other.length;
        blob = nullptr;
        length = 0;
        return *this;
    }

    /// Internal blob data
    uint8_t* blob{nullptr};

    /// Byte length of blob
    uint32_t length{0};
};
