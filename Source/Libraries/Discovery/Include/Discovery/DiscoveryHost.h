#pragma once

// Discovery
#include "IDiscoveryHost.h"

// Std
#include <vector>

class DiscoveryHost : public IDiscoveryHost {
public:
    void Register(IDiscoveryListener *listener) override;
    void Deregister(IDiscoveryListener *listener) override;
    void Enumerate(uint32_t *count, IDiscoveryListener **listeners) override;

private:
    std::vector<IDiscoveryListener*> listeners;
};
