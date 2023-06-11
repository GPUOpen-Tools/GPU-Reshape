#pragma once

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <map>
#include <vector>
#include <mutex>

/// Stores tracked objects with additional states
///  Additionally stores a unique identifier per state, as the key type may be recycled
///  at any moment.
template<typename T>
struct TrackedObject {
    struct LinearView {
        LinearView(std::mutex &mutex, Vector<T*>& object) : mutex(mutex), object(object) {
            mutex.lock();
        }

        ~LinearView() {
            mutex.unlock();
        }

        /// No copy
        LinearView(const LinearView&) = delete;
        LinearView(LinearView&&) = delete;

        typename Vector<T*>::iterator begin() {
            return object.begin();
        }

        typename Vector<T*>::iterator end() {
            return object.end();
        }

        typename Vector<T*>::const_iterator begin() const {
            return object.begin();
        }

        typename Vector<T*>::const_iterator end() const {
            return object.end();
        }

        T* operator[](size_t i) const {
            return object[i];
        }

        std::mutex &mutex;
        Vector<T*>& object;
    };

    TrackedObject(const Allocators& allocators) : linear(allocators) {
        
    }

    /// Add a new tracked object
    T* Add(T* object) {
        std::lock_guard<std::mutex> guard(mutex);
        return AddNoLock(object);
    }

    /// Add a new tracked object, not thread safe
    T* AddNoLock(T* object) {
        MapEntry entry;
        entry.state = object;
        entry.slotRelocation = static_cast<uint32_t>(linear.size());

        // Assign the unique id
        object->uid = uidCounter++;

        // Append
        linear.push_back(object);
        uidMap[object->uid] = entry;
        return object;
    }

    /// Remove an object
    void Remove(T* object) {
        std::lock_guard<std::mutex> guard(mutex);
        RemoveNoLock(object);
    }

    /// Remove an object without a lock
    void RemoveNoLock(T* object) {
        const MapEntry& entry = uidMap.at(object->uid);

        // Relocate entry with back
        if (entry.slotRelocation != linear.size() - 1) {
            MapEntry& newEntry = uidMap.at(linear.back()->uid);
            newEntry.slotRelocation = entry.slotRelocation;

            std::swap(linear[entry.slotRelocation], linear.back());
        }

        // Remove linear view
        linear.pop_back();

        // Remove from map
        uidMap.erase(object->uid);
    }

    T* GetFromUID(uint64_t uid, T* _default = nullptr) {
        auto it = uidMap.find(uid);
        if (it == uidMap.end()) {
            return _default;
        }

        return it->second.state;
    }

    LinearView GetLinear() {
        return LinearView(mutex, linear);
    }

    std::mutex& GetLock() {
        return mutex;
    }

private:
    struct MapEntry {
        T*       state;
        uint32_t slotRelocation;
    };

    /// Separate uid counter
    uint64_t uidCounter{0};

    /// Lookup
    std::map<uint64_t, MapEntry> uidMap;

    /// Linear traversal
    Vector<T*> linear;

    /// My data! The data!
    std::mutex mutex;
};
