#pragma once

// Automation
#include <Test/Automation/Pass/ApplicationInfo.h>
#include <Test/Automation/Pass/ITestPass.h>

// Common
#include <Common/ComRef.h>

// Forward declarations
class DiscoveryService;

class ApplicationPass : public ITestPass {
public:
    COMPONENT(ApplicationPass);

    /// Constructor
    /// \param info application info
    /// \param subPass sub-pass to execute after connections
    ApplicationPass(const ApplicationInfo& info, ComRef<ITestPass> subPass);

    /// Overrides
    bool Run() override;

private:
    /// Run as an executable
    bool RunExecutable();

    /// Run as a steam application
    bool RunSteam();

    /// Terminate the application
    void TerminateApplication(uint32_t processID);

private:
    /// Test sub-pass
    ComRef<ITestPass> subPass;

    /// Discovery service
    ComRef<DiscoveryService> discovery;

    /// App info
    ApplicationInfo info;
};
