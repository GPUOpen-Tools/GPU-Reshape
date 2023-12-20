// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <Services/HostResolver/Shared.h>

// Common
#include <Common/IPGlobalLock.h>

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
    if (!globalLock.Acquire(kSharedHostResolverMutexName, true)) {
        std::cerr << "Failed to open or create shared mutex '" << kSharedHostResolverMutexName << "'" << std::endl;

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
    // ! Disable timeout
    constexpr uint32_t kMaxLonelyElapsed = ~0u;

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
