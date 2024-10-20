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

#pragma once

// Discovery
#include <Services/Discovery/DiscoveryService.h>

class ConditionalDiscovery {
public:
    /// Constructor
    /// \param discovery discovery service to start
    /// \param condition start on condition
    ConditionalDiscovery(ComRef<DiscoveryService> discovery, bool condition) : discovery(discovery), condition(condition) {
        Start(condition);
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

    /// Start this conditional
    /// @param conditionRetry condition to retry
    void Start(bool conditionRetry = true) {
        if (condition || !conditionRetry) {
            return;
        }

        // Start discovery
        condition = true;
        discovery->Start();

        // Let the service catch up
        // TODO: Future me, please, there has to be a better way
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }

private:
    /// Discovery service
    ComRef<DiscoveryService> discovery;

    /// Start condition
    bool condition;
};
