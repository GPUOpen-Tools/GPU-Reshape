#pragma once

// Std
#include <cstdint>

namespace IL {
    /// Simple inline array for instructions
    template<typename T>
    struct InlineArray {
        /// Get element
        /// \param i index of element
        /// \return element
        T& operator[](uint32_t i) {
            return reinterpret_cast<T*>(&count + 1)[i];
        }

        /// Get element
        /// \param i index of element
        /// \return element
        const T& operator[](uint32_t i) const {
            return reinterpret_cast<const T*>(&count + 1)[i];
        }

        /// Get element size
        /// \param count number of elements
        /// \return byte size
        static uint64_t ElementSize(uint32_t count) {
            return sizeof(T) * count;
        }

        /// Get element size
        /// \return byte size
        uint64_t ElementSize() const {
            return sizeof(T) * count;
        }

        /// Number of elements
        uint32_t count;
    };
}
