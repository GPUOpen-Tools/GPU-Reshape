// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include "AllocatorTag.h"

// Std
#include <cstdlib>

/// Default allocator
inline void *AllocateDefault(void*, size_t size, size_t align, AllocationTag) {
    return malloc(size);
}

/// Default free
inline void FreeDefault(void*, void *ptr, size_t align) {
    free(ptr);
}

/// Allocator callbacks
using TAllocatorAlloc  = void*(*)(void* user, size_t size, size_t align, AllocationTag tag);
using TAllocatorFree = void(*)(void* user, void* ptr, size_t align);

/// Contains basic allocators
struct Allocators {
    void* userData{nullptr};

    /// Current allocation tag
    AllocationTag tag = kDefaultAllocTag;

    /// Allocate handler
    TAllocatorAlloc alloc = AllocateDefault;

    /// Free handler
    TAllocatorFree free = FreeDefault;

    /// Set the new tag
    /// \param _tag given tag
    /// \return new allocators
    Allocators Tag(const AllocationTag& _tag) const {
        Allocators self = *this;
        self.tag = _tag;
        return self;
    }

    /// Check for equality
    bool operator==(const Allocators& other) const {
        return userData == other.userData && alloc == other.alloc && free == other.free;
    }
};