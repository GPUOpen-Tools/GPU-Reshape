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
#include <Common/Allocator/Allocators.h>
#include <Common/Allocator/AllocatorTag.h>

// Std
#include <cstring>

/// Default alignment
static constexpr uint32_t kDefaultAlign = sizeof(void*);

/// Allocation overload
inline void* operator new (size_t size, const Allocators& allocators) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, allocators.tag);
}

/// Allocation overload
inline void* operator new[](size_t size, const Allocators& allocators) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, allocators.tag);
}

/// Allocation overload
inline void* operator new (size_t size, const Allocators& allocators, AllocationTag tag) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, tag);
}

/// Allocation overload
inline void* operator new[](size_t size, const Allocators& allocators, AllocationTag tag) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, tag);
}

/// Placement delete for exception handling
inline void operator delete (void* data, const Allocators& allocators) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete[](void* data, const Allocators& allocators) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete (void* data, const Allocators& allocators, AllocationTag) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete[](void* data, const Allocators& allocators, AllocationTag) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Destruction (can't overload deletion)
template<typename T>
inline void destroy(T* object, Allocators allocators) {
    if (!object)
        return;

    object->~T();

    // Poison fill memory
#ifndef NDEBUG
    memset(static_cast<void*>(object), 0xFFu, sizeof(T));
#endif
    
    allocators.free(allocators.userData, object, kDefaultAlign);
}