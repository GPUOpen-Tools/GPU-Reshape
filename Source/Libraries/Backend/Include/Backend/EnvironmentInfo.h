#pragma once

#include <Common/Allocators.h>

class IBridge;

namespace Backend {
    struct EnvironmentInfo {
        /// Allocators
        Allocators allocators;

        /// Optional, bridge override
        IBridge* bridge{nullptr};

        /// Load plugins?
        bool loadPlugins{true};
    };
}
