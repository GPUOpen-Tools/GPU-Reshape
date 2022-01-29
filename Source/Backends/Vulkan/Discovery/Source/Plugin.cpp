// Common
#include <Common/Registry.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Discovery
#include <Discovery/IDiscoveryHost.h>

// Vulkan
#include <Backends/Vulkan/VulkanDiscoveryListener.h>

static VulkanDiscoveryListener* listener{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "VulkanDiscovery";
    info->description = "Application discovery for vulkan";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto* host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return false;
    }

    // Install the listener
    listener = registry->New<VulkanDiscoveryListener>();
    host->Register(listener);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto* host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return;
    }

    // Uninstall the listener
    host->Deregister(listener);
    destroy(listener);
}
