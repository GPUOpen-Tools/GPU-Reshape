// Common
#include <Common/Registry.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Backend
#include <Bridge/IBridge.h>

// Schemas
#include <Schemas/Features/ResourceBounds.h>

// ResourceBounds
#include <Features/ResourceBounds/Listener.h>

static ComRef<ResourceBoundsListener> listener{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "ResourceBounds";
    info->description = "Presentation for resource bounds messages";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto bridge = registry->Get<IBridge>();
    if (!bridge) {
        return false;
    }

    // Install the resource bounds listener
    listener = registry->New<ResourceBoundsListener>();
    if (!listener->Install()) {
        return false;
    }

    // Register with bridge
    bridge->Register(ResourceIndexOutOfBoundsMessage::kID, listener);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto bridge = registry->Get<IBridge>();
    if (!bridge) {
        return;
    }

    // Uninstall the listener
    bridge->Deregister(ResourceIndexOutOfBoundsMessage::kID, listener);
    listener.Release();
}
