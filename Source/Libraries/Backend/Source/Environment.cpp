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

#include <Backend/Environment.h>
#include <Backend/EnvironmentInfo.h>
#include <Backend/FeatureHost.h>
#include <Backend/EnvironmentKeys.h>

// Bridge
#include <Bridge/MemoryBridge.h>
#include <Bridge/HostServerBridge.h>
#include <Bridge/Network/PingPongListener.h>

// Services
#include <Services/HostResolver/HostResolverService.h>

// Schemas
#include <Schemas/PingPong.h>

// Common
#include <Common/Alloca.h>
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Plugin/PluginResolver.h>

using namespace Backend;

static AsioHostClientToken GetReservedStartupAsioToken() {
    // Try to get length
    uint64_t length{0};
    if (getenv_s(&length, nullptr, 0u, kReservedEnvironmentTokenKey) || !length) {
        return {};
    }

    // Get path data
    auto* path = ALLOCA_ARRAY(char, length);
    if (getenv_s(&length, path, length, kReservedEnvironmentTokenKey)) {
        return {};
    }

    // Construct from value
    return GlobalUID::FromString(path);
} 

Environment::Environment() {
}

Environment::~Environment() {
    // Resolver may not be installed (uninitialized environments, valid usage)
    auto resolver = registry.Get<PluginResolver>();
    if (!resolver) {
        return;
    }

    // Uninstall all plugins
    resolver->Uninstall();
}

bool Environment::Install(const EnvironmentInfo &info) {
    // Install the plugin resolver
    auto resolver = registry.AddNew<PluginResolver>();

    // Install the dispatcher
    registry.AddNew<Dispatcher>();

    // Install bridge
    if (info.memoryBridge) {
        // Intra process
        registry.AddNew<MemoryBridge>();
    } else {
        // Install the host resolver
        //  ? Ensures that the host resolver is running on the system
        HostResolverService hostResolverService;
        if (!hostResolverService.Install()) {
            return false;
        }

        // Networked
        auto network = registry.AddNew<HostServerBridge>();
        
        // Endpoint info
        EndpointConfig endpointConfig;
        endpointConfig.applicationName = info.applicationName;
        endpointConfig.apiName = info.apiName;
        endpointConfig.reservedToken = GetReservedStartupAsioToken();

        // Attempt to install as server
        if (!network->Install(endpointConfig)) {
            return false;
        }

        // Add default ping pong listener
        network->Register(PingPongMessage::kID, registry.New<PingPongListener>(network.GetUnsafe()));
    }

    // Install feature host
    registry.AddNew<FeatureHost>();

    // Need plugins?
    if (info.loadPlugins) {
        // Find all plugins
        if (!resolver->FindPlugins("backend", &plugins, PluginResolveFlag::ContinueOnFailure)) {
            return false;
        }

        // Install all found plugins
        resolver->InstallPlugins(plugins);
    }

    // OK
    return true;
}
