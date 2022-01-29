// Discovery
#include <Services/Discovery/DiscoveryService.h>

// Common
#include <Common/Console/ConsoleDevice.h>

int main(int32_t argc, const char* const* argv) {
    DiscoveryService service;

    // Info
    std::cout << "Standalone Discovery Service\n\n";

    // Attempt to install
    std::cout << "Starting services... " << std::flush;
    if (!service.Install()) {
        std::cerr << "Failed to install service\n";
        return 1;
    }

    // OK
    std::cout << "OK." << std::endl;

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
