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

#include <Test/Automation/Pass/ApplicationPass.h>
#include <Test/Automation/Pass/ConditionalDiscovery.h>
#include <Test/Automation/Connection/Connection.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Data/ApplicationData.h>
#include <Test/Automation/Data/TestData.h>
#include <Test/Automation/Data/HistoryData.h>

// System
#ifdef _WIN32
#   include <Windows.h>
#else // _WIN32
#   error Not implemented
#endif // _WIN32

// Common
#include <Common/Win32Object.h>
#include <Common/RegistryScope.h>
#include <Common/String.h>

// Std
#include <regex>

ApplicationPass::ApplicationPass(const ApplicationInfo& info, ComRef<ITestPass> subPass): subPass(subPass), info(info) {

}

bool ApplicationPass::Run() {
    DiagnosticScope log(registry, "Application {0} ({1})", info.identifier, info.processName);

    // Enabled at all?
    if (!info.enabled) {
        Log(registry, "Disabled by tests");
        return true;
    }

    // Apply name filter
    if (ComRef testData = registry->Get<TestData>()) {
        if (!testData->applicationFilter.empty() &&
            !std::icontains(info.identifier, testData->applicationFilter) &&
            !std::icontains(info.processName, testData->applicationFilter)) {
            Log(registry, "Disabled by filter");
            return true;
        }
    }

    // Data for all sub-passes
    RegistryScope data(registry, registry->New<ApplicationData>());

    // Shared history
    ComRef history = registry->Get<HistoryData>();
    
    // Find discovery
    discovery = registry->Get<DiscoveryService>();
    if (!discovery) {
        Log(registry, "Missing discovery service");
        return false;
    }

    // Discovery disabled before actually needed
    ConditionalDiscovery discoveryGuard(discovery, false);

    // Get all identifiers
    std::vector<FilterEntry> filterEntries;
    FilterPaths(filterEntries);

    // Did any application fail?
    bool anyFailed = false;

    // Run against all identifiers
    for (const FilterEntry& filterEntry : filterEntries) {
        if (filterEntry.identifier != info.identifier) {
            Log(registry, "Filter: {0}", filterEntry.identifier);
        }

        // Check history
        uint64_t historyTag = StringCRC32Short(Format("Application:{0}", filterEntry.identifier).c_str());
        if (history && history->IsCompleted(historyTag)) {
            Log(registry, "Known good (history)");
            continue;
        }
        
        // Conditionally start discovery
        discoveryGuard.Start(info.requiresDiscovery);
        
        // Run application
        bool result = false;
        switch (info.type) {
            default:
                ASSERT(false, "Invalid type");
                break;
            case ApplicationLaunchType::Executable:
            case ApplicationLaunchType::ExecutableFilter:
                result = RunExecutable(filterEntry.identifier);
                break;
            case ApplicationLaunchType::Steam:
                result = RunSteam(filterEntry.identifier);
                break;
        }

        // OK?
        if (!result) {
            Log(registry, "Failed to launch");
            
            anyFailed = true;
            continue;
        }

        // Create connection
        RegistryScope connection(registry, registry->New<Connection>());

        // Try to install connection on localhost
        if (!connection->Install(EndpointResolve {
            .ipvxAddress = "127.0.0.1"
        }, filterEntry.processName)) {
            TerminateApplication(connection->GetProcessID());
            Log(registry, "Failed to connect to application");
            
            anyFailed = true;
            continue;
        }

        // Keep local PID
        data->processID = connection->GetProcessID();

        // Run all tests
        bool testResult = subPass->Run();
        if (!testResult) {
            anyFailed = true;
        }

        // Let the application run for a bit, validate its stability
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));

        // Check if the application didn't crash somewhere
        if (!data->IsAlive()) {
            Log(registry, "Application crashed during tests");

            anyFailed = true;
            continue;
        }

        // Tests done, terminate
        Log(registry, "Terminating");
        TerminateApplication(data->processID);
        
        // Check history
        if (history && testResult) {
            history->Complete(historyTag);
        }
    } 
    
    // OK?
    return !anyFailed;
}

bool ApplicationPass::RunExecutable(const std::string& identifier) {
    // Convert to absolute
    std::string path = std::filesystem::absolute(identifier).string();

    // Get working directory
    std::string workingDirectory = info.workingDirectory.empty() ? std::filesystem::path(path).parent_path().string() : info.workingDirectory;

    // No startup environment
    MessageStream stream;

    // Ignored info
    DiscoveryProcessInfo processInfo{};

    // Try to launch as a bootstrapped process
    DiscoveryProcessCreateInfo createInfo{};
    createInfo.applicationPath = path.c_str();
    createInfo.workingDirectoryPath = workingDirectory.c_str();
    createInfo.arguments = info.arguments.c_str();
    return discovery->StartBootstrappedProcess(createInfo, stream, processInfo);
}

bool ApplicationPass::RunSteam(const std::string& identifier) {
    // Launch string
    std::string arguments = "steam://rungameid/" + identifier;

    // Append arguments if any
    if (!info.arguments.empty()) {
        arguments += "//" + info.arguments + "/";
    }

    // Assume steam path from environment
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "STEAM_EXE") || !buf) {
        return false;
    }

    // Launch as shell command
    ShellExecuteA(nullptr, "open", buf, arguments.c_str(), nullptr, SW_SHOWMINIMIZED);
    return true;
}

void ApplicationPass::TerminateApplication(uint32_t processID) {
#if _WIN32
    // Try to open process
    Win32Handle process = OpenProcess(PROCESS_TERMINATE, false, processID);
    if (!process) {
        return;
    }

    // Terminate and wait
    TerminateProcess(process, 0u);
    WaitForSingleObject(process, static_cast<DWORD>(std::chrono::milliseconds(10000).count()));
#else // _WIN32
#error Not implemented
#endif // _WIN32

    // Small delay to let the OS clean up handles
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ApplicationPass::FilterPaths(std::vector<FilterEntry> &identifiers) {
    switch (info.type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case ApplicationLaunchType::Executable:
        case ApplicationLaunchType::Steam:
            identifiers.push_back(FilterEntry {
                .identifier = info.identifier,
                .processName = info.processName
            });
            break;
        case ApplicationLaunchType::ExecutableFilter:
            FilterExecutables(identifiers);
            break;
    }
}

static std::regex GetWildcardRegex(const std::string& wildcard) {
    std::string regex;
    regex.reserve(wildcard.size() + 32);

    // Handle wildcards and escaped characters
    for (char character : wildcard) {
        switch (character) {
            default: {
                regex.push_back(character);
                break;
            }
            case '*': {
                regex.append(".*");
                break;
            }
            case '?': {
                regex.push_back('.');
                break;
            }
            case '\\':
            case '.':
            case '^':
            case '|':
            case '[':
            case ']':
            case '{':
            case '}': {
                regex.push_back('\\');
                regex.push_back(character);
                break;
            }
        }
    }
    
    return std::regex(regex);
}

void ApplicationPass::FilterExecutables(std::vector<FilterEntry> &identifiers) {
    std::filesystem::path path      = info.identifier;
    std::filesystem::path directory = path.parent_path();

    // Get regex
    std::regex regex = GetWildcardRegex(path.filename().string());

    // Match entire directory
    for (auto&& entry: std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), regex)) {
            identifiers.push_back(FilterEntry {
                .identifier = entry.path().string(),
                .processName = entry.path().filename().string()
            });
        }
    }
}
