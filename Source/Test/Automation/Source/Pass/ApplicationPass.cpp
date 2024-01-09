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

// Common
#include <Common/Win32Object.h>
#include <Common/RegistryScope.h>
#include <Common/String.h>

ApplicationPass::ApplicationPass(const ApplicationInfo& info, ComRef<ITestPass> subPass): subPass(subPass), info(info) {

}

bool ApplicationPass::Run() {
    DiagnosticScope log(registry, "Application {0}", info.processName);

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

    // Find discovery
    discovery = registry->Get<DiscoveryService>();
    if (!discovery) {
        Log(registry, "Missing discovery service");
        return false;
    }

    // Conditionally enable discovery
    ConditionalDiscovery discoveryGuard(discovery, info.requiresDiscovery);

    // Run application
    bool result = false;
    switch (info.type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case ApplicationLaunchType::Executable:
            result = RunExecutable();
            break;
        case ApplicationLaunchType::Steam:
            result = RunSteam();
            break;
    }

    // OK?
    if (!result) {
        Log(registry, "Failed to launch");
        return false;
    }

    // Create connection
    RegistryScope connection(registry, registry->New<Connection>());

    // Try to install connection on localhost
    if (!connection->Install(EndpointResolve {
        .ipvxAddress = "127.0.0.1"
    }, info.processName)) {
        TerminateApplication(connection->GetProcessID());
        Log(registry, "Failed to connect to application");
        return false;
    }

    // Keep local PID
    data->processID = connection->GetProcessID();

    // Run all tests
    result = subPass->Run();

    // Check if the application didn't crash somewhere
    if (!data->IsAlive()) {
        Log(registry, "Application crashed during tests");
        return false;
    }

    // Tests done, terminate
    Log(registry, "Terminating");
    TerminateApplication(data->processID);

    // OK
    return result;
}

bool ApplicationPass::RunExecutable() {
    // Convert to absolute
    std::string path = std::filesystem::absolute(info.identifier).string();

    // Get working directory
    std::string workingDirectory = std::filesystem::path(path).parent_path().string();

    // No startup environment
    MessageStream stream;

    // Try to launch as a bootstrapped process
    DiscoveryProcessInfo processInfo{};
    processInfo.applicationPath = path.c_str();
    processInfo.workingDirectoryPath = workingDirectory.c_str();
    processInfo.arguments = info.arguments.c_str();
    return discovery->StartBootstrappedProcess(processInfo, stream);
}

bool ApplicationPass::RunSteam() {
    // Launch string
    std::string arguments = "steam://rungameid/" + info.identifier;

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
