// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <mutex>

template<typename T>
class EventHandler {
public:
    /// Invoke all handlers
    template<typename... A>
    void Invoke(A&&... args) {
        std::lock_guard guard(mutex);

        for (auto&& kv : subscriptions) {
            kv.second(std::forward<A>(args)...);
        }
    }

    /// Add a handler
    /// \param uid owner id of handler
    /// \param delegate invokable delegate
    void Add(uint64_t uid, const T& delegate) {
        std::lock_guard guard(mutex);
        subscriptions.emplace_back(uid, delegate);
    }

    /// Remove a handler
    /// \param uid registered id of handler
    /// \return false if not found
    bool Remove(uint64_t uid) {
        std::lock_guard guard(mutex);

        auto it = std::find(subscriptions.begin(), subscriptions.end(), [uid](auto&& kv) { return kv.first == uid; });
        if (it == subscriptions.end()) {
            return false;
        }

        // Remove it
        subscriptions.erase(it);
        return true;
    }

private:
    /// Shared lock
    std::mutex mutex;

    /// All subscriptions
    std::vector<std::pair<uint64_t, T>> subscriptions;
};
