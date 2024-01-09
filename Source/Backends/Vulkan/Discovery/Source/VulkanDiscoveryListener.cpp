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

#include <Backends/Vulkan/VulkanDiscoveryListener.h>
#include <Backends/Vulkan/Layer.h>

// Discovery
#include "Discovery/DiscoveryBootstrappingEnvironment.h"

// Common
#include <Common/EnvironmentArray.h>
#include <Common/FileSystem.h>

// System
#ifdef _WIN32
#   include <Windows.h>
#else
#   error Not implemented
#endif

#ifdef _WIN32
bool QueryImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GRS.json";

    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        key,
        path,
        0, nullptr, 0,
        KEY_READ, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Attempt to query key, if it already exists, another listener is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        layerJsonPath.wstring().c_str(),
        RRF_RT_DWORD,
        nullptr,
        nullptr, 0
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Path is present
    return true;
}

bool FindConflictingImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GRS.json";

    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        key,
        path,
        0, nullptr, 0,
        KEY_READ, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Number of values
    DWORD valueCount{0};

    // Query key count
    error = RegQueryInfoKey(
        keyHandle,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valueCount,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // For each key
    for (uint32_t i = 0; i < valueCount; i++) {
        char buffer[1024];

        // Size of data, replaced
        DWORD bufferSize = sizeof(buffer);

        // Query key name
        error = RegEnumValue(
            keyHandle,
            i,
            buffer,
            &bufferSize,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );

        // OK?
        if (error == ERROR_SUCCESS) {
            std::string keyName(buffer, bufferSize / sizeof(char));

            // Bad layer?
            if (keyName.find("VK_LAYER_GPUOPEN_GRS.json") != std::string::npos && keyName != layerJsonPath) {
                return true;
            }
        }
    }

    // No bad instance
    return false;
}

bool UninstallConflictingImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GRS.json";

    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        key,
        path,
        0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return true;
    }

    // Number of values
    DWORD valueCount{0};

    // Query key count
    error = RegQueryInfoKey(
        keyHandle,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &valueCount,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Keys to remove
    std::vector<std::string> bucket;

    // For each key
    for (uint32_t i = 0; i < valueCount; i++) {
        char buffer[1024];

        // Size of data, replaced
        DWORD bufferSize = sizeof(buffer);

        // Query key name
        error = RegEnumValue(
            keyHandle,
            i,
            buffer,
            &bufferSize,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );

        // OK?
        if (error == ERROR_SUCCESS) {
            std::string keyName(buffer, bufferSize / sizeof(char));

            // Bad layer?
            if (keyName.find("VK_LAYER_GPUOPEN_GRS.json") != std::string::npos && keyName != layerJsonPath) {
                bucket.push_back(keyName);
            }
        }
    }

    // Delete all keys
    for (const std::string& key : bucket) {
        error = RegDeleteValue(
            keyHandle,
            key.c_str()
        );

        // OK?
        if (error != ERROR_SUCCESS) {
            return false;
        }
    }

    // OK
    return true;
}

bool InstallImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GRS.json";

    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        key,
        path,
        0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Attempt to query key, if it already exists, another listener is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        layerJsonPath.wstring().c_str(),
        RRF_RT_DWORD,
        nullptr,
        nullptr, 0
    );
    if (error == ERROR_SUCCESS) {
        return true;
    }

    // Value properties
    DWORD value{0};

    // Not found, so attempt to set the value
    error = RegSetValueExW(
        keyHandle,
        layerJsonPath.wstring().c_str(), 0, REG_DWORD,
        (const BYTE *)&value, sizeof(value)
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}

bool UninstallImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GRS.json";

    // Open properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        key,
        path,
        0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        // Master doesn't exist, ok
        return true;
    }

    // Attempt to query key, may not exist
    error = RegGetValueW(
        keyHandle,
        nullptr,
        layerJsonPath.wstring().c_str(),
        RRF_RT_DWORD,
        nullptr,
        nullptr, 0
    );
    if (error != ERROR_SUCCESS) {
        // Layer doesn't exist, ok
        return true;
    }

    // Delete the implicit layer key
    error = RegDeleteValueW(
        keyHandle,
        layerJsonPath.wstring().c_str()
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}
#endif

VulkanDiscoveryListener::VulkanDiscoveryListener() {
    // Query current status
    isGlobal |= QueryImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    isGlobal |= QueryImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
}

bool VulkanDiscoveryListener::InstallGlobal() {
    // Attempt to install implicit non-administrator layer
    if (!InstallImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // Attempt to install implicit administrator layer, optional success
    InstallImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");

    // Attached!
    isGlobal = true;

    // OK
    return true;
}

bool VulkanDiscoveryListener::UninstallGlobal() {
    // Attempt to install implicit non-administrator layer
    if (!UninstallImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // Attempt to install implicit administrator layer
    if (!UninstallImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // No longer attached
    isGlobal = false;

    // OK
    return true;
}

bool VulkanDiscoveryListener::HasConflictingInstances() {
    // Check user
    if (FindConflictingImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return true;
    }

    // Check machine
    if (FindConflictingImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return true;
    }

    // OK
    return false;
}

bool VulkanDiscoveryListener::UninstallConflictingInstances() {
    bool anyFailed = false;
    anyFailed |= !UninstallConflictingImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    anyFailed |= !UninstallConflictingImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    return !anyFailed;
}

bool VulkanDiscoveryListener::Start() {
    // Attempt to install implicit non-administrator layer
    if (!InstallImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // Attempt to install implicit administrator layer, optional success
    InstallImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");

    // OK
    return true;
}

bool VulkanDiscoveryListener::Stop() {
    // Global listener is attached, handled elsewhere
    if (isGlobal) {
        return true;
    }

    // Attempt to install implicit non-administrator layer
    if (!UninstallImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // Attempt to install implicit administrator layer
    if (!UninstallImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // OK
    return true;
}

void VulkanDiscoveryListener::SetupBootstrappingEnvironment(const DiscoveryProcessInfo &info, DiscoveryBootstrappingEnvironment &env) {
    // Add search path and layer
    env.environmentKeys.emplace_back("VK_LAYER_PATH", GetCurrentExecutableDirectory().string());
    env.environmentKeys.emplace_back("VK_INSTANCE_LAYERS", VK_GPUOPEN_GPURESHAPE_LAYER_NAME);
}

bool VulkanDiscoveryListener::IsGloballyInstalled() {
    if (!isGlobal) {
        return false;
    }

    // Validate that the implicit layers are active
    return QueryImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers") ||
           QueryImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
}

bool VulkanDiscoveryListener::IsRunning() {
    bool isRunning = false;
    isRunning |= QueryImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    isRunning |= QueryImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    return isRunning;
}
