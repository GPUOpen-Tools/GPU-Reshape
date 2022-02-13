#pragma once

// Bridge
#include "AsioDebug.h"

// Std
#include <cstdint>

/// Default resolving port, must be unique
static constexpr uint16_t kDefaultAsioHostResolvePort = 34'213;

/// General config
struct AsioConfig {
    /// Shared resolve port
    uint16_t hostResolvePort = kDefaultAsioHostResolvePort;
};

/// Remote endpoint config
struct AsioRemoteConfig {
    /// Shared resolve port
    uint16_t hostResolvePort = kDefaultAsioHostResolvePort;

    /// Address to connect to
    const char* ipvxAddress{nullptr};
};
