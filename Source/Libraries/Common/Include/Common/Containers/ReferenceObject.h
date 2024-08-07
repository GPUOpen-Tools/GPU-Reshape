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
#include <Common/Assert.h>
#include <Common/Allocators.h>

// Std
#include <atomic>
#ifndef __cplusplus_cli
#include <mutex>
#endif // __cplusplus_cli

// Forward declarations
struct ReferenceObject;

/// Host state for a reference object
#ifndef __cplusplus_cli
struct ReferenceHost {
    /// Shared mutex
    /// Any operation that may result in a new user must be guarded by this
    std::mutex mutex;
};
#else // __cplusplus_cli
/// CoreRT threading workaround
struct ReferenceHost;
#endif // __cplusplus_cli

/// A reference counted object
struct ReferenceObject {
    ReferenceObject() {

    }

    virtual ~ReferenceObject() {
        // Ensure the object is fully released
        ASSERT(users.load() == 0, "Dangling users to referenced object, use destroyRef");
    }
    
    /// Release all host resources
    /// Must be called when locked
    virtual void ReleaseHost() {
        /** poof */
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

    /// Get the number of users
    uint32_t GetUsers() const {
        return users.load();
    }

    /// Optional reference host
    ReferenceHost* referenceHost{nullptr};
    
private:
    /// Number of users for this object
    std::atomic<uint32_t> users{0};
};

namespace Detail {
    /// Lock the owning reference host
    void LockReferenceHost(ReferenceHost* host);

    /// Unlock the owning reference host
    void UnlockReferenceHost(ReferenceHost* host);
}

template<typename T>
inline void destroyRef(T* object, const Allocators& allocators) {
    if (!object->ReleaseUserNoDestruct()) {
        return;
    }
    
    // If there's a host, synchronize it
    if (ReferenceHost* host = object->referenceHost) {
        Detail::LockReferenceHost(host);

        // With the host acquired, ensure no user has been added
        // If so, the object remains alive
        if (object->GetUsers()) {
            Detail::UnlockReferenceHost(host);
            return;
        }

        // Inform the host of the release
        object->ReleaseHost();
        Detail::UnlockReferenceHost(host);
    }

    destroy(object, allocators);
}

template<typename T>
inline constexpr bool IsReferenceObject = std::is_base_of_v<
    ReferenceObject,
    std::remove_const_t<std::remove_pointer_t<std::remove_reference_t<T>>>
>;
