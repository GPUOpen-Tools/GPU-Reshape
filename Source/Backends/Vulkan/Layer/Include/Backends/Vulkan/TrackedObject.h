#pragma once

// Std
#include <map>
#include <mutex>

/// Stores tracked objects with additional states
template<typename T, typename U>
struct TrackedObject {
    /// Add a new tracked object
    U* Add(T object, U* state) {
        std::lock_guard<std::mutex> guard(mutex);
        return AddNoLock(object, state);
    }

    /// Add a new tracked object, not thread safe
    U* AddNoLock(T object, U* state) {
        map[object] = state;
        return state;
    }

    /// Get a tracked object
    U* Get(T object) {
        std::lock_guard<std::mutex> guard(mutex);
        return GetNoLock(object);
    }

    /// Get a tracked object, not thread safe
    U* GetNoLock(T object) {
        return map.at(object);
    }

private:
    std::map<T, U*> map;
    std::mutex      mutex;
};
