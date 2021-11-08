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
