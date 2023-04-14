#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <string_view>

class VulkanDiscoveryListener final : public IDiscoveryListener {
public:
    VulkanDiscoveryListener();

    /// Overrides
    bool IsRunning() override;
    bool IsGloballyInstalled() override;
    bool Start() override;
    bool Stop() override;
    void SetupBootstrappingEnvironment(const DiscoveryProcessInfo& info, DiscoveryBootstrappingEnvironment& env) override;
    bool InstallGlobal() override;
    bool UninstallGlobal() override;
    bool HasConflictingInstances() override;
    bool UninstallConflictingInstances() override;

private:
    /// Is this listener presently globally installed?
    bool isGlobal{false};
};
