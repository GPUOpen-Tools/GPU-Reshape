#pragma once

// Message
#include "MessageStream.h"

/// Find a message in a stream
/// \tparam T expected type
/// \param view ordered view
/// \return nullptr if not found
template<typename T>
const T* Find(const MessageStreamView<>& view) {
    for (auto it = view.GetIterator(); it; ++it) {
        if (it.Is(T::kID)) {
            return it.Get<T>();
        }
    }

    // Not found
    return nullptr;
}

/// Find a message in a stream
/// \tparam T expected type
/// \param view ordered view
/// \return nullptr if not found
template<typename T>
T FindOrDefault(const MessageStreamView<>& view, const T& _default) {
    for (auto it = view.GetIterator(); it; ++it) {
        if (it.Is(T::kID)) {
            return *it.Get<T>();
        }
    }

    // Not found
    return _default;
}
