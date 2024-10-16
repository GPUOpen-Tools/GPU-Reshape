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

// Automation
#include <Test/Automation/Pass/ApplicationLaunchType.h>

// Std
#include <vector>
#include <string>

struct ApplicationInfo {
    /// Type of the application
    ApplicationLaunchType type{ApplicationLaunchType::None};

    /// Is the application enabled?
    /// Tests are skipped otherwise
    bool enabled{true};

    /// Does this application require discovery for successful attaching?
    bool requiresDiscovery{false};

    /// Process name for attaching
    std::string processName;

    /// Launch identifier, see launch type for details
    std::string identifier;

    /// Threshold before the connection kicks in
    uint32_t connectionObjectThreshold{1u};

    /// All command line arguments passed to the application
    std::vector<std::string> arguments;

    /// Optional, working directory
    std::string workingDirectory;

    /// Steam payload
    struct {
        /// Local path to execute within the application path
        std::string path;
    } steam;
};
