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
#include <cstdint>
#include <vector>
#include <mutex>
#include <map>

/// Simple dependency tracker
/// TODO: Make the search faster...
template<typename T, typename U>
struct DependentObject {
    struct Object {
        std::vector<U*> dependencies;
    };

    struct ObjectView {
        ObjectView(std::mutex &mutex, Object& object) : mutex(mutex), object(object) {
            mutex.lock();
        }

        ~ObjectView() {
            mutex.unlock();
        }

        /// No copy
        ObjectView(const ObjectView&) = delete;
        ObjectView(ObjectView&&) = delete;

        typename std::vector<U*>::iterator begin() {
            return object.dependencies.begin();
        }

        typename std::vector<U*>::iterator end() {
            return object.dependencies.end();
        }

        typename std::vector<U*>::const_iterator begin() const {
            return object.dependencies.begin();
        }

        typename std::vector<U*>::const_iterator end() const {
            return object.dependencies.end();
        }

        std::mutex &mutex;
        Object& object;
    };

    /// Add an object dependency
    /// \param key the dependent object
    /// \param value the dependency
    void Add(T* key, U* value) {
        std::lock_guard<std::mutex> guard(mutex);

        Object& obj = map[key];
        obj.dependencies.push_back(value);
    }

    /// Remove an object dependency
    /// \param key the dependent object
    /// \param value the dependency
    void Remove(T* key, U* value) {
        std::lock_guard<std::mutex> guard(mutex);
        Object& obj = map[key];

        // Find the value
        auto&& it = std::find(obj.dependencies.begin(), obj.dependencies.end(), value);
        if (it == obj.dependencies.end()) {
            return;
        }

        // Swap with back
        if (*it != obj.dependencies.back()) {
            *it = obj.dependencies.back();
        }

        // Pop
        obj.dependencies.pop_back();
    }

    /// Get all dependencies
    ObjectView Get(T* key) {
        return ObjectView(mutex, map[key]);
    }

    /// Get the number of dependencies
    uint32_t Count(T* key) {
        return static_cast<uint32_t>(map[key].dependencies.size());
    }

private:
    std::mutex           mutex;
    std::map<T*, Object> map;
};
