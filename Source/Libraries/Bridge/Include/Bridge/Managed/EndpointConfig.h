#pragma once

using namespace System;

// Bridge
#include <Bridge/EndpointConfig.h>

namespace Bridge::CLR {
    public ref class EndpointConfig {
    public:
        /// Shared port
        UInt32 sharedPort = kBridgeSharedPort;

        /// Name of connecting application
        String^ applicationName = "Unknown";
    };

    public ref class EndpointResolve {
    public:
        /// Shared config
        EndpointConfig^ config;

        /// Endpoint address
        String^ ipvxAddress = "";
    };
}
