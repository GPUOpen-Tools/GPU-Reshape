// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Services/Discovery/DiscoveryService.h>
#include <Services/Discovery/NotifyIconDiscoveryListener.h>

// Backend
#include <Backend/EnvironmentKeys.h>
#include <Backend/StartupEnvironment.h>

// Discovery
#include <Discovery/DiscoveryHost.h>
#include <Discovery/IDiscoveryListener.h>
#include <Discovery/DiscoveryBootstrappingEnvironment.h>

// Common
#include <Common/EnvironmentArray.h>

// Detour
#ifdef _WIN32
#include <Detour/detours.h>
#endif // _WIN32

DiscoveryService::~DiscoveryService() {
    if (resolver) {
        resolver->Uninstall();
    }
}

bool DiscoveryService::Install() {
    // Create the host
    host = registry.AddNew<DiscoveryHost>();

    // Create the resolver
    resolver = registry.AddNew<PluginResolver>();

    // Find all plugins
    PluginList list;
    if (!resolver->FindPlugins("discovery", &list)) {
        return false;
    }

    // Attempt to install all plugins
    if (!resolver->InstallPlugins(list)) {
        return false;
    }

    // Get the number of listeners
    uint32_t listenerCount{0};
    host->Enumerate(&listenerCount, nullptr);

    // Get all listeners
    listeners.resize(listenerCount);
    host->Enumerate(&listenerCount, listeners.data());

    // Add notify icon listener
    listeners.push_back(registry.New<NotifyIconDiscoveryListener>());

    // OK
    return true;
}

bool DiscoveryService::IsGloballyInstalled() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->IsGloballyInstalled()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::IsRunning() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->IsRunning()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::Start() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Start()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::Stop() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Stop()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::InstallGlobal() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->InstallGlobal()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::UninstallGlobal() {
    bool anyFailed = false;

    // Uninstall all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->UninstallGlobal()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::HasConflictingInstances() {
    bool anyBad = false;

    // Check all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (listener->HasConflictingInstances()) {
            anyBad = true;
        }
    }

    // OK
    return anyBad;
}

bool DiscoveryService::UninstallConflictingInstances() {
    bool anyFailed = false;

    // Check all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->UninstallConflictingInstances()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::StartBootstrappedProcess(const DiscoveryProcessInfo &info, const MessageStream &environment) {
    DiscoveryBootstrappingEnvironment bootstrappingEnvironment;

    // Write the startup environment
    bootstrappingEnvironment.environmentKeys.emplace_back(Backend::kStartupEnvironmentKey, Backend::StartupEnvironment{}.WriteEnvironment(environment));

    // Write token if valid
    if (info.reservedToken.IsValid()) {
        bootstrappingEnvironment.environmentKeys.emplace_back(Backend::kReservedEnvironmentTokenKey, info.reservedToken.ToString());
    }
    
    // Compose environment
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        listener->SetupBootstrappingEnvironment(info, bootstrappingEnvironment);
    }
    
#ifdef _WIN32
    // Startup info
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_SHOW;
    
    // Process info
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    // Copy arguments
    // Note: Win32 always has the path as the first argument
    std::stringstream argumentStream;
    argumentStream << "\"" << info.applicationPath << "\"";
    argumentStream << " ";
    argumentStream << info.arguments;

    // Load environment set
    EnvironmentArray environmentArray;

    // Append all expected environment variables
    for (const std::pair<std::string, std::string>& kv : bootstrappingEnvironment.environmentKeys) {
        environmentArray.Add(kv.first, kv.second);
    }

    // Flatten environment
    std::string environmentBlock = environmentArray.ComposeEnvironmentBlock();

    // Flatten dll keys
    std::vector<const char*> dllKeys;
    for (const std::string& key : bootstrappingEnvironment.dlls) {
        dllKeys.push_back(key.c_str());
    }
    
    // Attempt to create process
    if (!DetourCreateProcessWithDllsA(
        info.applicationPath,
        argumentStream.str().data(),
        0x0,
        0x0,
        false,
        DETACHED_PROCESS,
        environmentBlock.data(),
        info.workingDirectoryPath,
        &startupInfo, &processInfo,
        static_cast<uint32_t>(dllKeys.size()), dllKeys.data(), nullptr
    )) {
        return false;
    }

    // Begin!
    ResumeThread(processInfo.hThread);
#else // _WIN32
#   error Not implemented
#endif // _WIN32

    // OK
    return true;
}
