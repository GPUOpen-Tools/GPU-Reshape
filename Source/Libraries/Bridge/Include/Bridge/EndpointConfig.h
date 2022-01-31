#pragma once

// Std
#include <cstdint>

static constexpr uint32_t kBridgeDefaultEndpointPort = 34'213;

struct EndpointConfig {
    /// Port of the endpoint
    uint32_t port = kBridgeDefaultEndpointPort;
};

struct EndpointResolve {
    /// Configuration of the endpoint
    EndpointConfig config;

    /// IP address
    const char* ipvxAddress{nullptr};
};
