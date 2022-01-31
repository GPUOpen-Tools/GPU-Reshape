// Discovery
#include <Services/Discovery/DiscoveryService.h>

// Schemas
#include <Schemas/Log.h>

// Common
#include <Common/Console/ConsoleDevice.h>

// Bridge
#include <Bridge/NetworkBridge.h>
#include <Bridge/Log/LogConsoleListener.h>

int main(int32_t argc, const char* const* argv) {
    DiscoveryService service;

    // Info
    std::cout << "Standalone Discovery Service\n\n";

    // Attempt to install service
    std::cout << "Starting services... " << std::flush;
    {
        if (!service.Install()) {
            std::cerr << "Failed to install service\n";
            return 1;
        }

        // OK
        std::cout << "OK." << std::endl;
    }

    // Get registry
    Registry* registry = service.GetRegistry();

    // TODO: This is quite ugly... (needs releasing too)
    std::thread commitThread;

    // Attempt to start bridge
    std::cout << "Starting network bridge... " << std::flush;
    {
        auto* network = registry->AddNew<NetworkBridge>();

        // Set up resolve
        EndpointResolve resolve;
        resolve.ipvxAddress = "127.0.0.1";

        // Attempt to install as client
        if (!network->InstallClient(resolve)) {
            std::cerr << "Failed to start network bridge\n";
            return 1;
        }

        // Install log listener
        auto* logListener = registry->New<LogConsoleListener>();
        network->Register(LogMessage::kID, logListener);

        // Commit helper
        commitThread = std::thread([&] {
            for (;;) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                network->Commit();
            }
        });

        // OK
        std::cout << "OK." << std::endl;
    }

    // Start device
    ConsoleDevice console;
    while (console.Next()) {
        if (console.Is("ping")) {
            std::cout << "pong" << std::endl;
        } else {
            std::cout << "Unknown command '" << console.Command() << "'" << std::endl;
        }
    }

    // OK
    return 0;
}
