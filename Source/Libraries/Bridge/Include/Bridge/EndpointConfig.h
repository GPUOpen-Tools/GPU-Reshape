#pragma once

// Std
#include <cstdint>

static constexpr uint32_t kBridgeSharedPort = 34'213;

struct EndpointConfig {
    /// Shared port for resolving
    uint32_t sharedPort = kBridgeSharedPort;

    /// Name of the application
    const char* applicationName{"Unknown"};

    /// Name of the API
    const char* apiName{"Unknown"};
};

struct EndpointResolve {
    /// Configuration of the endpoint
    EndpointConfig config;

    /// IP address
    const char* ipvxAddress{nullptr};
};
