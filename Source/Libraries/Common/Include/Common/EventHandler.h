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
