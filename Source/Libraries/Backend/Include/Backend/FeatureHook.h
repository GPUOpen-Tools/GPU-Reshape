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

// Backend
#include <Common/Delegate.h>

// Forward declarations
class CommandContext;

/// Feature hook, derived type must implement operator()
template<typename T>
struct TFeatureHook {
    using Hook = T;

    /// Backend hook
    Hook hook;
};

/// Feature hook, derived type must implement operator()
template<typename R, typename... A>
struct TFeatureHook<R(A...)> {
    using Hook = Delegate<R(A...)>;

    /// Backend hook
    Hook hook;
};

/// Apply a given feature hook
/// \tparam T the feature typename, see the specification above
/// \param featureBitSet the active bit set
/// \param featureHooks the feature hooks registered
/// \param args all hook arguments
template<typename T, typename O, typename... A>
inline bool ApplyFeatureHook(O* object, CommandContext* context, uint64_t featureBitSet, typename T::Hook featureHooks[64], A... args) {
    // Early out if empty
    if (!featureBitSet)
        return false;

    // Current mask
    uint64_t bitMask = featureBitSet;

    // Scan for all set hooks
    unsigned long index;
    while (_BitScanReverse64(&index, bitMask)) {
        // Forward the hook to the appropriate handler
        T handler;
        handler.hook = featureHooks[index];
        handler(object, context, args...);

        // Next!
        bitMask &= ~(1ull << index);
    }

    // OK
    return true;
}

