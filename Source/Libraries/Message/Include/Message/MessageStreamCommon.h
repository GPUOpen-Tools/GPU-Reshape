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
T FindOrDefault(const MessageStreamView<>& view, const T& _default = {}) {
    for (auto it = view.GetIterator(); it; ++it) {
        if (it.Is(T::kID)) {
            return *it.Get<T>();
        }
    }

    // Not found
    return _default;
}

/// Collapse a message type in a stream
/// \tparam T expected type
/// \param view ordered view
/// \return collapsed message
template<typename T>
T CollapseOrDefault(const MessageStreamView<>& view, T _default = {}) {
    for (auto it = view.GetIterator(); it; ++it) {
        if (it.Is(T::kID)) {
            _default |= *it.Get<T>();
        }
    }

    // OK
    return _default;
}
