#pragma once

// Backend
#include <Backend/Delegate.h>

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
template<typename T, typename... A>
inline void ApplyFeatureHook(uint64_t featureBitSet, typename T::Hook featureHooks[64], A... args) {
    // Early out if empty
    if (!featureBitSet)
        return;

    // Current mask
    unsigned long bitMask = featureBitSet;

    // Scan for all set hooks
    unsigned long index;
    while (_BitScanReverse(&index, bitMask)) {
        // Forward the hook to the appropriate handler
        T{featureHooks[index]}(args...);

        // Next!
        bitMask &= ~(1ul << index);
    }
}

