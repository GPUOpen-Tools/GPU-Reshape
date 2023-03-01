#pragma once

// Common
#include <Common/Common.h>

// Forward declarations
class Registry;

// Forward declarations
struct PluginInfo;

/// Plugin info type
using PluginInfoDelegate = void(*)(PluginInfo* info);

/// Plugin installation type
using PluginInstallDelegate = bool(*)(Registry* registry);

/// Plugin de-installation type
using PluginUninstallDelegate = void(*)(Registry* registry);

/// Plugin entrypoint names
#define PLUGIN_INFO GetPluginInfo
#define PLUGIN_INSTALL InstallPlugin
#define PLUGIN_UNINSTALL UninstallPlugin

/// Plugin entrypoint strings
#define PLUGIN_INFO_S "GetPluginInfo"
#define PLUGIN_INSTALL_S "InstallPlugin"
#define PLUGIN_UNINSTALL_S "UninstallPlugin"
