#pragma once

// Std
#include <vector>

/// Object pool for trivially copyable objects
template<typename T>
struct TrivialObjectPool {
    /// Try to pop an object
    /// \param out filled object
    /// \return true if successful
    bool TryPop(T& out) {
        if (objects.empty()) {
            return false;
        }

        out = objects.back();
        objects.pop_back();
        return true;
    }

    /// Pop an object from this pool, or create a new one
    /// \return the popped object
    T Pop() {
        if (objects.empty()) {
            return T{};
        }

        T obj = objects.back();
        objects.pop_back();
        return obj;
    }

    /// Push an object to this pool
    /// \param obj
    void Push(const T& obj) {
        objects.push_back(obj);
    }

private:
    std::vector<T> objects;
};