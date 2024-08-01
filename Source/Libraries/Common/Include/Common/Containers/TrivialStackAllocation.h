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

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Single-use stack allocation, with optional heap fallback
template<size_t STACK_LENGTH>
struct TrivialStackAllocation {
    /// Constructor
    TrivialStackAllocation(const Allocators& allocators = {}) : container(allocators) {
        /** */
    }

    /// Allocate from this container
    /// Invalidates any previous allocation
    /// \param length expected byte length
    /// \return data
    void* Alloc(uint64_t length) {
        container.Resize(length);
        return container.Data();
    }

    /// Allocate from this container
    /// Invalidates any previous allocation
    /// \param length expected byte length
    /// \return data
    template<typename T>
    T* Alloc(uint64_t length) {
        return static_cast<T*>(Alloc(length));
    }

    /// Allocate an array from this container
    /// Invalidates any previous allocation
    /// \param count number of elements in the array
    /// \return data
    template<typename T>
    T* AllocArray(uint64_t count) {
        return static_cast<T*>(Alloc(sizeof(T) * count));
    }

private:
    /// Underlying container
    TrivialStackVector<uint8_t, STACK_LENGTH> container;
};
