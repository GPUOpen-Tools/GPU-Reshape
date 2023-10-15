// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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

    /// Number of elements
    /// \return count
    size_t Size() const {
        return elements.size() * 32u;
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
