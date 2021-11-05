#pragma once

// Common
#include <Common/Assert.h>

// Std
#include <atomic>

/// A reference counted object
struct ReferenceObject {
    ~ReferenceObject() {
        // Ensure the object is fully released
        ASSERT(users.load() == 0, "Dangling users to referenced object, use ReferenceObject::Release");
    }

    /// Add a user to this object
    void AddUser() {
        users++;
    }

    /// Release a user to this object
    /// \return true if all users have been deleted
    bool ReleaseUser() {
        return --users == 0;
    }

private:
    /// Number of users for this object, starts at 1 for base allocation
    std::atomic<uint32_t> users{1};
};