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

// Common
#include <Common/Registry.h>
#include <Common/Plugin/PluginList.h>

// Forward declarations
class HostServerBridge;

namespace Backend {
    struct EnvironmentInfo;
    struct EnvironmentDeviceInfo;

    /// Standard backend environment
    class Environment {
    public:
        Environment();
        ~Environment();

        /// Install this environment
        /// \param info initialization information
        /// \return success state
        bool Install(const EnvironmentInfo& info);

        /// Update this environment
        /// \param info update info
        void Update(const EnvironmentDeviceInfo& info);

        /// Get the registry of this environment
        /// \return
        Registry* GetRegistry() {
            return &registry;
        }

    private:
        /// Internal registry
        Registry registry;

        /// Optional, host server bridge
        ComRef<HostServerBridge> hostServerBridge;

        /// Listed plugins
        PluginList plugins;

        /// Last update hash
        uint64_t deviceUpdateHash{0};
    };
}