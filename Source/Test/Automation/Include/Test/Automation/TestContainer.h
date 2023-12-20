#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

// Common
#include <Common/ComRef.h>
#include <Common/IComponent.h>

// Forward declarations
class HostResolverService;
class DiscoveryService;

class TestContainer : public IComponent {
public:
    /// Install this container
    /// \return success state
    bool Install();

    /// Run a test in this container
    /// \return success state
    bool Run(ComRef<ITestPass> pass);

private:
    /// Container host resolver
    ComRef<HostResolverService> hostResolver;

    /// Container discovery
    ComRef<DiscoveryService> discovery;
};
