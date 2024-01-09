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
