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
inline constexpr bool IsReferenceObject = std::is_base_of_v<ReferenceObject, std::remove_const_t<std::remove_reference_t<T>>>;
