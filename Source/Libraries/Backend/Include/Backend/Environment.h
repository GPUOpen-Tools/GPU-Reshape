#pragma once

// Common
#include <Common/Registry.h>
#include <Common/Plugin/PluginList.h>

namespace Backend {
    struct EnvironmentInfo;

    /// Standard backend environment
    class Environment {
    public:
        Environment();
        ~Environment();

        /// Install this environment
        /// \param info initialization information
        /// \return success state
        bool Install(const EnvironmentInfo& info);

        /// Get the registry of this environment
        /// \return
        Registry* GetRegistry() {
            return &registry;
        }

    private:
        /// Internal registry
        Registry registry;

        /// Listed plugins
        PluginList plugins;
    };
}