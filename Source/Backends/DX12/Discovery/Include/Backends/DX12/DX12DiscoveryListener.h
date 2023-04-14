#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <string_view>
#include <filesystem>

class DX12DiscoveryListener final : public IDiscoveryListener {
public:
    DX12DiscoveryListener();

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
    /// Start the service process
    /// \return success state
    bool StartProcess();

    /// Stop the service process
    /// \return success state
    bool StopProcess();
    
private:
    /// Is this listener presently globally installed?
    bool isGlobal{false};

    /// Backend path
    std::filesystem::path servicePath;
};
