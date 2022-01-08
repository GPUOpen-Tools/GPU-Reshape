#pragma once

// Std
#include <string>
#include <vector>

struct PluginInfo {
    /// Name of this plugin
    std::string name;

    /// Description of this plugin
    std::string description;

    /// Any inter-plugin dependencies
    std::vector<std::string> dependencies;
};
