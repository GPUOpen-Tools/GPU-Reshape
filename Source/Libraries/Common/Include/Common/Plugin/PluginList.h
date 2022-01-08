#pragma once

// Common
#include "PluginInfo.h"

// Std
#include <string>
#include <vector>

struct PluginEntry {
    /// The handle name
    std::string plugin;

    /// Info of the plugin
    PluginInfo info;
};

struct PluginList {
    /// All plugin
    std::vector<PluginEntry> plugins;
};
