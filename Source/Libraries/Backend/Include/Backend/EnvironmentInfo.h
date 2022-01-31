#pragma once

#include <Common/Allocators.h>

class IBridge;

namespace Backend {
    struct EnvironmentInfo {
        /// Allocators
        Allocators allocators;

        /// In memory bridge?
        bool memoryBridge{false};

        /// Load plugins?
        bool loadPlugins{true};
    };
}
