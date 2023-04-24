#pragma once

// Common
#include <Common/IComponent.h>

// Std
#include <string>
#include <vector>

struct FeatureInfo {
    /// Name of the feature
    std::string name;

    /// General information
    std::string description;

    /// Instrumentation dependencies
    std::vector<ComponentID> dependencies;
};
