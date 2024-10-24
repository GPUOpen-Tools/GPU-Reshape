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
#include <Test/Automation/Steam/VDFHeader.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
class ConditionalDiscovery;
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
    /// Filter entry data
    struct FilterEntry {
        std::string identifier;
        std::string processName;
    };
    
    /// Run a test instance
    bool RunInstance(ConditionalDiscovery &discoveryGuard, const FilterEntry& filterEntry, const std::string& arguments);

private:
    /// Run as an executable
    bool RunExecutable(const std::string& identifier, const std::string& arguments);

    /// Run as a steam application
    bool RunSteam(const std::string& identifier, const std::string& arguments);

    /// Run from a local steam path
    bool RunSteamPath(const std::string_view& libraryPath, VDFDictionaryNode* manifest, const std::string& identifier, const std::string& arguments);

    /// Run from a steam id
    bool RunSteamID(const std::string& steamPath, VDFDictionaryNode* manifest, const std::string& identifier, const std::string& arguments);

    /// Terminate the application
    void TerminateApplication(uint32_t processID);

private:
    /// Get the path of steam
    bool GetSteamPath(std::string& out);
    
    /// Try to get the steam library folders
    bool GetSteamLibraryFolders(VDFArena& arena, const std::string& steamPath, const std::string& identifier, VDFDictionaryNode** out);
    
    /// Try to find the application library path
    bool GetSteamAppLibraryPath(VDFArena& arena, const std::string& steamPath, const std::string& identifier, std::string& libraryPath);
    
    /// Try to get the application manifest
    bool GetSteamAppManifest(VDFArena& arena, const std::string& libraryPath, const std::string& identifier, VDFDictionaryNode** out);

private:
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
