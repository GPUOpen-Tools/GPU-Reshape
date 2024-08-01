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

// Common
#include <Common/Containers/ReferenceObject.h>

// Std
#include <map>
#include <vector>
#include <mutex>

/// Stores tracked objects with additional states
///  Additionally stores a unique identifier per state, as the key type may be recycled
///  at any moment.
template<typename T, typename U>
struct TrackedObject : public ReferenceHost {
    struct LinearView {
        LinearView(std::mutex &mutex, std::vector<U*>& object) : mutex(mutex), object(object) {
            mutex.lock();
        }

        ~LinearView() {
            mutex.unlock();
        }

        /// No copy
        LinearView(const LinearView&) = delete;
        LinearView(LinearView&&) = delete;

        typename std::vector<U*>::iterator begin() {
            return object.begin();
        }

        typename std::vector<U*>::iterator end() {
            return object.end();
        }

        typename std::vector<U*>::const_iterator begin() const {
            return object.begin();
        }

        typename std::vector<U*>::const_iterator end() const {
            return object.end();
        }

        U* operator[](size_t i) const {
            return object[i];
        }

        std::mutex &mutex;
        std::vector<U*>& object;
    };

    /// Add a new tracked object
    U* Add(T object, U* state) {
        std::lock_guard<std::mutex> guard(mutex);
        return AddNoLock(object, state);
    }

    /// Add a new tracked object, not thread safe
    U* AddNoLock(T object, U* state) {
        MapEntry entry;
        entry.state = state;
        entry.slotRelocation = static_cast<uint32_t>(linear.size());

        // Assign the unique id
        state->uid = uidCounter++;

        // Set the host
        if constexpr (IsReferenceObject<U>) {
            ASSERT(!state->referenceHost, "Reference host double assignment");
            state->referenceHost = this;   
        }

        // Append
        linear.push_back(state);
        map[object] = state;
        uidMap[state->uid] = entry;
        return state;
    }

    /// Get a tracked object
    U* Get(T object) {
        std::lock_guard<std::mutex> guard(mutex);
        return GetNoLock(object);
    }

    /// Get a tracked object
    U* TryGet(T object) {
        std::lock_guard<std::mutex> guard(mutex);
        return TryGetNoLock(object);
    }

    /// Get a tracked object, not thread safe
    U* GetNoLock(T object) {
        return map.at(object);
    }

    /// Get a tracked object, not thread safe
    U* TryGetNoLock(T object) {
        auto it = map.find(object);
        return it != map.end() ? it->second : nullptr;
    }

    /// Remove an object
    void Remove(T object, U* state) {
        RemoveLogical(object);
        RemoveState(state);
    }

    /// Remove an object
    void Remove(T object) {
        RemoveState(Get(object));
        RemoveLogical(object);
    }

    /// Remove an object
    void RemoveLogical(T object) {
        std::lock_guard<std::mutex> guard(mutex);

        // Remove from map
        map.erase(object);
    }

    /// Remove an object
    void RemoveState(U* state) {
        std::lock_guard<std::mutex> guard(mutex);
        RemoveStateNoLock(state);
    }

    /// Remove an object
    void RemoveStateNoLock(U* state) {
        const MapEntry& entry = uidMap.at(state->uid);

        // Relocate entry with back
        if (entry.slotRelocation != linear.size() - 1) {
            MapEntry& newEntry = uidMap.at(linear.back()->uid);
            newEntry.slotRelocation = entry.slotRelocation;

            std::swap(linear[entry.slotRelocation], linear.back());
        }

        // Remove linear view
        linear.pop_back();

        // Remove from map
        uidMap.erase(state->uid);
    }

    U* GetFromUID(uint64_t uid, U* _default = nullptr) {
        std::lock_guard guard(mutex);
        
        auto it = uidMap.find(uid);
        if (it == uidMap.end()) {
            return _default;
        }

        return it->second.state;
    }

    /// Get the number of objects
    size_t GetCount() {
        std::lock_guard guard(mutex);
        return linear.size();
    }

    LinearView GetLinear() {
        return LinearView(mutex, linear);
    }

    std::mutex& GetLock() {
        return mutex;
    }

private:
    struct MapEntry {
        U*       state;
        uint32_t slotRelocation;
    };

    /// Separate uid counter
    uint64_t uidCounter{0};

    /// Lookup
    std::map<T, U*>  map;
    std::map<uint64_t, MapEntry> uidMap;

    /// Linear traversal
    std::vector<U*> linear;
};
