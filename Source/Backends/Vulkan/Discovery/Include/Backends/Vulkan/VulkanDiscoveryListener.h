#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <string>

class VulkanDiscoveryListener final : public IDiscoveryListener {
public:
    ~VulkanDiscoveryListener();

    /// Overrides
    bool Install() override;

private:
    bool InstallImplicitLayer();
    void UninstallImplicitLayer();

private:
    std::wstring key;

    // Does this listener own the key?
    bool keyOwner{false};
};
