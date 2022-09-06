#pragma once

// Discovery
#include <Discovery/IDiscoveryListener.h>

// Std
#include <string_view>

class DX12DiscoveryListener final : public IDiscoveryListener {
public:
    /// Overrides
    bool Install() override;
    bool Uninstall() override;
};
