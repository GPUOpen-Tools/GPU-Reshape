#pragma once

// Discovery
#include <Services/Discovery/DiscoveryService.h>

class ConditionalDiscovery {
public:
    /// Constructor
    /// \param discovery discovery service to start
    /// \param condition start on condition
    ConditionalDiscovery(ComRef<DiscoveryService> discovery, bool condition) : discovery(discovery), condition(condition) {
        if (condition) {
            discovery->Start();

            // Let the service catch up
            // TODO: Future me, please, there has to be a better way
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        }
    }

    /// Deconstructor
    ~ConditionalDiscovery() {
        if (condition) {
            discovery->Stop();

            // Let the service catch up
            // TODO: Future me, please, there has to be a better way
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        }
    }

private:
    /// Discovery service
    ComRef<DiscoveryService> discovery;

    /// Start condition
    bool condition;
};
