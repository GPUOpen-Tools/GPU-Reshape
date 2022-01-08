#pragma once

// Std
#include <vector>

// Common
#include <Common/Allocators.h>
#include <Common/Assert.h>

/// Indirect pool of objects
template<typename T>
struct ObjectPool {
    ObjectPool(Allocators allocators = Allocators()) : allocators(allocators) {
        // ...
    }

    ~ObjectPool() {
        ASSERT(count == 0, "Dangling object references not free'd to ObjectPool");

        // Free all objects
        for (T* obj : pool) {
            destroy(obj, allocators);
        }
    }

    /// Pop a new object, construct if none found
    /// \param args construction arguments
    /// \return the object
    template<typename... A>
    T* Pop(A&&... args) {
#ifndef NDEBUG
        count++;
#endif

        if (!pool.empty()) {
            T* obj = pool.back();
            pool.pop_back();
            return obj;
        }

        return new (allocators) T(args...);
    }

    /// Try to pop a new object
    /// \return nullptr if none found
    T* TryPop() {
        if (pool.empty()) {
            return nullptr;
        }

        T* obj = pool.back();
        pool.pop_back();
        return obj;
    }

    /// Pop a new object, always construct even if found
    /// \param args construction arguments
    /// \return the constructed object
    template<typename... A>
    T* PopConstruct(A&&... args) {
#ifndef NDEBUG
        count++;
#endif

        if (!pool.empty()) {
            T* obj = pool.back();
            pool.pop_back();

            obj->~T();

            return new (obj) T(args...);
        }

        return new (allocators) T(args...);
    }

    /// Push an object to this pool
    /// \param object the object
    void Push(T* object) {
#ifndef NDEBUG
        count--;
#endif

        pool.push_back(object);
    }

private:
    Allocators allocators;

    /// Pool of objects
    std::vector<T*> pool;

    /// Debug
#ifndef NDEBUG
    uint32_t count{0};
#endif
};
