#pragma once

// Common
#include <Common/Allocator/Vector.h>

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
        Object(const Allocators& allocators) : dependencies(allocators) {
            
        }
        
        Vector<U*> dependencies;
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

        typename Vector<U*>::iterator begin() {
            return object.dependencies.begin();
        }

        typename Vector<U*>::iterator end() {
            return object.dependencies.end();
        }

        typename Vector<U*>::const_iterator begin() const {
            return object.dependencies.begin();
        }

        typename Vector<U*>::const_iterator end() const {
            return object.dependencies.end();
        }

        std::mutex &mutex;
        Object& object;
    };

    DependentObject(const Allocators& allocators) : allocators(allocators) {
        
    }

    /// Add an object dependency
    /// \param key the dependent object
    /// \param value the dependency
    void Add(T* key, U* value) {
        std::lock_guard<std::mutex> guard(mutex);

        Object& obj = GetObj(key);
        obj.dependencies.push_back(value);
    }

    /// Remove an object dependency
    /// \param key the dependent object
    /// \param value the dependency
    void Remove(T* key, U* value) {
        std::lock_guard<std::mutex> guard(mutex);
        Object& obj = GetObj(key);

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
        return ObjectView(mutex, GetObj(key));
    }

    /// Get the number of dependencies
    uint32_t Count(T* key) {
        return static_cast<uint32_t>(GetObj(key).dependencies.size());
    }

private:
    Object& GetObj(T* key) {
        auto it = map.find(key);
        if (it == map.end()) {
            it = map.emplace(key, allocators).first;
        }

        return it->second;
    }

private:
    std::mutex           mutex;
    std::map<T*, Object> map;

private:
    Allocators allocators;
};
