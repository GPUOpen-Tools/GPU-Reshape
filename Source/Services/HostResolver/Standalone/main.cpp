#include <Services/HostResolver/Shared.h>
#include <Services/HostResolver/IPGlobalLock.h>

// Bridge
#include <Bridge/Asio/AsioHostResolverServer.h>

// Std
#include <cstdint>
#include <iostream>

int main(int32_t argc, const char* const* argv) {
    std::cout << "GPUOpen Host Resolver\n" << std::endl;
    std::cout << "Initializing global lock... " << std::flush;

    // Try to acquire lock
    IPGlobalLock globalLock;
    if (!globalLock.Acquire(true)) {
        std::cerr << "Failed to open or create shared mutex '" << kWin32SharedMutexName << "'" << std::endl;

#ifndef NDEBUG
        std::cin.ignore();
#endif
        return 1;
    }

    std::cout << "OK." << std::endl;

    std::cout << "Initializing server ... " << std::flush;

    // Try to open the server
    AsioConfig config{};
    AsioHostResolverServer server(config);

    // Success?
    if (!server.IsOpen()) {
        std::cerr << "Failed to open host resolve server at port " << config.hostResolvePort << std::endl;
        return 1;
    }

    std::cout << "OK.\n" << std::endl;
    std::cout << "Sever started" << std::endl;

    // Seconds before closure
    constexpr uint32_t kMaxLonelyElapsed = 30;

    // Number of seconds without any connections
    uint32_t elapsedLonelyServer = 0;

    // Hold while the server remains open
    for (; server.IsOpen();) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Any connections?
        if (!server.GetServer().GetConnectionCount()) {
            std::cout << "No connections... " << elapsedLonelyServer << "/" << kMaxLonelyElapsed << std::endl;

            if (elapsedLonelyServer++ >= kMaxLonelyElapsed) {
                // Send stop signal
                server.Stop();
                break;
            }
        } else {
            // Reset
            elapsedLonelyServer = 0;
        }
    }

    // OK
    std::cout << "Host resolver shutdown" << std::endl;
    return 0;
}
