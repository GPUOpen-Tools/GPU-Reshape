// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

        /// Get the underlying data
        T* Data() {
            return reinterpret_cast<T*>(&count + 1);
        }

        /// Get the underlying data
        const T* Data() const {
            return reinterpret_cast<const T*>(&count + 1);
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
