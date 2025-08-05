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

// Backend
#include <Backend/Device/DeviceState.h>

// Common
#include <Common/IComponent.h>

// Std
#include <vector>

class IDeviceStateVote : public TComponent<IDeviceStateVote> {
public:
    COMPONENT(DeviceStateVote);

    /// Register a new vote
    /// @param uid allocated uid
    /// @param value value to vote for
    template<typename T>
    void Register(uint64_t uid, const T& value) {
        TypeBucket& bucket = buckets[static_cast<uint32_t>(T::kType)];

        // Insert new value
        DeviceStateVariant variant;
        reinterpret_cast<T&>(variant) = value;
        bucket.values.emplace_back(uid, variant);

        // Collapse against existing
        MergeValue(bucket, value);

        // Notify owner
        OnStateChanged(T::kType);
    }

    /// Deregister a vote
    /// @param uid uid used on Register
    template<typename T>
    void Deregister(uint64_t uid) {
        TypeBucket& bucket = buckets[static_cast<uint32_t>(T::kType)];

        // Find the state
        if (auto it = std::find_if(bucket.values.begin(), bucket.values.end(), [&](const ValuePair& value) {
            return value.first == uid;
        }); it != bucket.values.end()) {
            bucket.values.erase(it);

            // Recreate collapsed
            Resummarize<T>(bucket);

            // Notify owner
            OnStateChanged(T::kType);
        }
    }

    /// Get an existing (voted) state or get the default
    /// @return state
    template<typename T>
    T GetOrDefault() {
        TypeBucket& bucket = buckets[static_cast<uint32_t>(T::kType)];

        if (bucket.variant.type != DeviceStateType::None) {
            return reinterpret_cast<T&>(bucket.variant);
        }

        return T{};
    }

    /// Allocate a new identifier
    /// Doesn't need to be free'd
    uint64_t AllocateUID() {
        return uid++;
    }

protected:
    /// Invoked on state changes
    /// @param type state that was changed
    virtual void OnStateChanged(DeviceStateType type) {
        /** poof */
    }

private:
    using ValuePair = std::pair<uint64_t, DeviceStateVariant>;
    
    struct TypeBucket {
        /// Collapsed vote
        DeviceStateVariant variant;

        /// All vote values
        std::vector<ValuePair> values;
    };

    /// Resummarize an entire bucket
    template<typename T>
    void Resummarize(TypeBucket& bucket) {
        bucket.variant.type = DeviceStateType::None;

        // Merge all contained votes
        for (auto&& [uid, value] : bucket.values) {
            MergeValue<T>(bucket, reinterpret_cast<const T&>(value));
        }
    }

    /// Merge a single vote against a bucket
    template<typename T>
    void MergeValue(TypeBucket& bucket, const T& value) {
        T& dest = reinterpret_cast<T&>(bucket.variant);

        // If no existing vote, overwrite
        if (bucket.variant.type == DeviceStateType::None) {
            dest = value;
        } else {
            dest |= value;
        }
    }

    /// All buckets
    TypeBucket buckets[static_cast<uint32_t>(DeviceStateType::Count)];

private:
    /// Monotonically incrementing id
    uint64_t uid = 0;
};
