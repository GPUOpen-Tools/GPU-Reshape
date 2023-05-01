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
