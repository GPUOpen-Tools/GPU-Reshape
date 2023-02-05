#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <filesystem>

class NotifyIconDiscoveryListener final : public IDiscoveryListener {
public:
    NotifyIconDiscoveryListener();

    /// Overrides
    bool IsRunning() override;
    bool IsGloballyInstalled() override;
    bool Start() override;
    bool Stop() override;
    bool InstallGlobal() override;
    bool UninstallGlobal() override;
    bool HasConflictingInstances() override;
    bool UninstallConflictingInstances() override;

private:
    /// Backend path
    std::filesystem::path notifyPath;
};
