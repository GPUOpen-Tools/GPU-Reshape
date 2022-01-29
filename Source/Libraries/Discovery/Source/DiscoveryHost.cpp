#include <Discovery/DiscoveryHost.h>

void DiscoveryHost::Register(IDiscoveryListener *listener) {
    listeners.push_back(listener);
}

void DiscoveryHost::Deregister(IDiscoveryListener *listener) {
    auto&& it = std::find(listeners.begin(), listeners.end(), listener);
    if (it == listeners.end()) {
        listeners.erase(it);
    }
}

void DiscoveryHost::Enumerate(uint32_t *count, IDiscoveryListener **_listeners) {
    if (_listeners) {
        std::copy(listeners.begin(), listeners.begin() + *count, _listeners);
    } else {
        *count = static_cast<uint32_t>(listeners.size());
    }
}
