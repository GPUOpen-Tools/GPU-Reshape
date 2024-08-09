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
#include <Test/Automation/Pass/ApplicationInfo.h>
#include <Test/Automation/Pass/ITestPass.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
class DiscoveryService;

class ApplicationPass : public ITestPass {
public:
    COMPONENT(ApplicationPass);

    /// Constructor
    /// \param info application info
    /// \param subPass sub-pass to execute after connections
    ApplicationPass(const ApplicationInfo& info, ComRef<ITestPass> subPass);

    /// Overrides
    bool Run() override;

private:
    /// Run as an executable
    bool RunExecutable(const std::string& identifier);

    /// Run as a steam application
    bool RunSteam(const std::string& identifier);

    /// Terminate the application
    void TerminateApplication(uint32_t processID);

private:
    /// Filter entry data
    struct FilterEntry {
        std::string identifier;
        std::string processName;
    };

    /// Filter all paths
    void FilterPaths(std::vector<FilterEntry>& identifiers);

    /// Filter all executables
    void FilterExecutables(std::vector<FilterEntry>& identifiers);

private:
    /// Test sub-pass
    ComRef<ITestPass> subPass;

    /// Discovery service
    ComRef<DiscoveryService> discovery;

    /// App info
    ApplicationInfo info;
};
