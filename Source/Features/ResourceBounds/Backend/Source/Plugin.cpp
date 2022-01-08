// Common
#include <Common/Registry.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Backend
#include <Backend/IFeatureHost.h>

// ResourceBounds
#include <Features/ResourceBounds/Feature.h>

static ResourceBoundsFeature* feature{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "ResourceBounds";
    info->description = "Instrumentation and validation of resource indexing operations";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto* host = registry->Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // Install the resource bounds feature
    feature = registry->New<ResourceBoundsFeature>();
    host->Register(feature);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto* host = registry->Get<IFeatureHost>();
    if (!host) {
        return;
    }

    // Uninstall the feature
    host->Deregister(feature);
    destroy(feature);
}
