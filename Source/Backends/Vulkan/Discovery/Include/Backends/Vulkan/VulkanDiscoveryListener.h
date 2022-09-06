#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <string_view>

class VulkanDiscoveryListener final : public IDiscoveryListener {
public:
    /// Overrides
    bool Install() override;
    bool Uninstall() override;
};
