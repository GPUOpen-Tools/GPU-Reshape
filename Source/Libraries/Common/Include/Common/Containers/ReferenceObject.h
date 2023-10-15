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

// Common
#include <Common/Assert.h>
#include <Common/Allocators.h>

// Std
#include <atomic>

/// A reference counted object
struct ReferenceObject {
    ReferenceObject() {

    }

    virtual ~ReferenceObject() {
        // Ensure the object is fully released
        ASSERT(users.load() == 0, "Dangling users to referenced object, use destroyRef");
    }

    /// Add a user to this object
    void AddUser() {
        users++;
    }

    /// Release a user to this object
    /// \return true if all users have been deleted
    bool ReleaseUserNoDestruct() {
        ASSERT(users.load() != 0, "No user to release");
        return --users == 0;
    }

private:
    /// Number of users for this object
    std::atomic<uint32_t> users{0};
};

template<typename T>
inline void destroyRef(T* object, const Allocators& allocators) {
    if (object->ReleaseUserNoDestruct()) {
        destroy(object, allocators);
    }
}

template<typename T>
inline constexpr bool IsReferenceObject = std::is_base_of_v<
    ReferenceObject,
    std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>
>;
