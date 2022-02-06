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
        // Free all objects
        for (T* obj : pool) {
            if constexpr(IsReferenceObject<T>) {
                destroyRef(obj, allocators);
            } else {
                destroy(obj, allocators);
            }
        }
    }

    /// Pop a new object, construct if none found
    /// \param args construction arguments
    /// \return the object
    template<typename... A>
    T* Pop(A&&... args) {
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
        pool.push_back(object);
    }

    typename std::vector<T*>::iterator begin() {
        return pool.begin();
    }

    typename std::vector<T*>::iterator end() {
        return pool.end();
    }

    typename std::vector<T*>::const_iterator begin() const {
        return pool.begin();
    }

    typename std::vector<T*>::const_iterator end() const {
        return pool.end();
    }

private:
    Allocators allocators;

    /// Pool of objects
    std::vector<T*> pool;
};
