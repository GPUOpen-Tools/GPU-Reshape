#pragma once

// Discovery
#include "IDiscoveryHost.h"

// Std
#include <vector>

class DiscoveryHost : public IDiscoveryHost {
public:
    void Register(const ComRef<IDiscoveryListener>&listener) override;
    void Deregister(const ComRef<IDiscoveryListener>&listener) override;
    void Enumerate(uint32_t *count, ComRef<IDiscoveryListener>*listeners) override;

private:
    std::vector<ComRef<IDiscoveryListener>> listeners;
};
