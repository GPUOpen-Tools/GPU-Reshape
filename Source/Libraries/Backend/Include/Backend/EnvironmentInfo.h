#pragma once

#include <Common/Allocators.h>

class IBridge;

namespace Backend {
    struct EnvironmentInfo {
        /// Allocators
        Allocators allocators;

        /// Application name
        const char* applicationName{"Unknown"};

        /// API name
        const char* apiName{"Unknown"};

        /// In memory bridge?
        bool memoryBridge{false};

        /// Load plugins?
        bool loadPlugins{true};
    };
}
