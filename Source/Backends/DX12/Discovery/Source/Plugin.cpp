// Common
#include <Common/Registry.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Discovery
#include <Discovery/IDiscoveryHost.h>

// DX12
#include <Backends/DX12/DX12DiscoveryListener.h>

static ComRef<DX12DiscoveryListener> listener{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "DX12Discovery";
    info->description = "Application discovery for DX12";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return false;
    }

    // Install the listener
    listener = registry->New<DX12DiscoveryListener>();
    host->Register(listener);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return;
    }

    // Uninstall the listener
    host->Deregister(listener);
    listener.Release();
}
