#pragma once

// Automation
#include <Test/Automation/Pass/ApplicationLaunchType.h>

// Std
#include <string>

struct ApplicationInfo {
    /// Type of the application
    ApplicationLaunchType type{ApplicationLaunchType::None};

    /// Is the application enabled?
    /// Tests are skipped otherwise
    bool enabled{true};

    /// Does this application require discovery for successful attaching?
    bool requiresDiscovery{false};

    /// Process name for attaching
    std::string processName;

    /// Launch identifier, see launch type for details
    std::string identifier;

    /// All command line arguments passed to the application
    std::string arguments;
};
