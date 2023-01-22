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
inline void ApplyFeatureHook(O* object, CommandContext* context, uint64_t featureBitSet, typename T::Hook featureHooks[64], A... args) {
    // Early out if empty
    if (!featureBitSet)
        return;

    // Current mask
    uint64_t bitMask = featureBitSet;

    // Scan for all set hooks
    unsigned long index;
    while (_BitScanReverse64(&index, bitMask)) {
        // Forward the hook to the appropriate handler
        T{featureHooks[index]}(object, context, args...);

        // Next!
        bitMask &= ~(1ull << index);
    }
}

