#pragma once

// Std
#include <string>

namespace Backend {
    struct PluginInfo {
        /// Name of this plugin
        std::string name;

        /// Description of this plugin
        std::string description;
    };
}
