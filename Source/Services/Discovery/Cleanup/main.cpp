// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

// Discovery
#include <Services/Discovery/DiscoveryService.h>
#include <Discovery/IDiscoveryListener.h>

// Std
#include <iostream>
#include <vector>

int main(int32_t argc, const char *const *argv) {
    DiscoveryService service;
    
    // Info
    std::cout << "GPUReshape - Discovery Cleanup Tool\n\n";

    // No arguments
    if (argc != 1u) {
        std::cout << "Invalid command line usage, tool takes no arguments\n";
        return 1u;
    }

    // Did any pass fail?
    bool anyFailed = false;
    
    // Initialize the discovery service container
    std::cout << "Initializing container... ";
    {
        // Note: This is not *service* installation, this is the container
        if (service.Install()) {
            std::cout << "OK." << std::endl;
        } else {
            std::cout << "Failed!" << std::endl;
            anyFailed = true;
        }
    }

    // Enumerate all listeners
    uint32_t listenerCount{0};
    service.EnumerateListeners(&listenerCount, nullptr);
    
    std::vector<IDiscoveryListener*> listeners(listenerCount);
    service.EnumerateListeners(&listenerCount, listeners.data());

    // Stop all instance based services
    std::cout << "Stopping running instances... ";
    {
        bool success = true;
        for (IDiscoveryListener* listener : listeners) {
            success &= listener->Stop();
        }

        if (success) {
            std::cout << "OK." << std::endl;
        } else {
            std::cout << "Failed!" << std::endl;
            anyFailed = true;
        }
    }

    // Uninstall all services
    std::cout << "Uninstalling all services... ";
    {
        bool success = true;
        for (IDiscoveryListener* listener : listeners) {
            success &= listener->UninstallGlobal();
        }

        if (success) {
            std::cout << "OK." << std::endl;
        } else {
            std::cout << "Failed!" << std::endl;
            anyFailed = true;
        }
    }

    // Success?
    if (anyFailed) {
        std::cout << "One or more passes failed." << std::endl;
    } else {
        std::cout << "Success." << std::endl;
    }

    // OK
    return anyFailed;
}
