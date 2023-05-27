#pragma once

// Std
#include <vector>

class BitArray {
public:
    struct Element {
        /// Implicit boolean
        operator bool() const {
            return element & bit;
        }

        /// Assign value
        /// \param value value to assign
        /// \return
        Element& operator=(bool value) {
            element &= ~bit;
            element |= value * bit;
            return *this;
        }

        /// Underlying element
        uint32_t& element;

        /// Specific bit
        uint32_t bit;
    };

    struct ConstElement {
        /// Implicit boolean
        operator bool() const {
            return element & bit;
        }

        /// Underlying element
        const uint32_t& element;

        /// Specific bit
        uint32_t bit;
    };

    /// Constructor
    BitArray() = default;

    /// Constructor
    /// \param size number of bits
    BitArray(size_t size) {
        Resize(size);
    }

    /// Acquire a bit, if the bit is already set, acquisition will fail
    /// \param i bit to acquire
    /// \return true if acquired
    bool Acquire(size_t i) {
        Element bit = operator[](i);
        if (bit) {
            return false;
        }

        bit = true;
        return true;
    }

    /// Resize this array
    /// \param size number of bits
    void Resize(size_t size) {
        elements.resize((size + 31) / 32);
    }

    /// Get an element
    /// \param i bit index
    /// \return element
    Element operator[](size_t i) {
        return {
            .element = elements[i / 32],
            .bit = 1u << (i % 32u)
        };
    }

    /// Get an element
    /// \param i bit index
    /// \return element
    ConstElement operator[](size_t i) const {
        return {
            .element = elements[i / 32],
            .bit = 1u << (i % 32u)
        };
    }

private:
    /// All elements
    std::vector<uint32_t> elements;
};
