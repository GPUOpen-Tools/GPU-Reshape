#pragma once

// Common
#include <Common/Registry.h>

namespace Backend {
    // Forward declarations
    struct PluginInfo;

    /// Plugin info type
    using BackendPluginInfo = void(PluginInfo* info);

    /// Plugin installation type
    using BackendPluginInstall = bool(*)(Registry* registry);

    /// Plugin de-installation type
    using BackendPluginUninstall = void(*)(Registry* registry);
}
