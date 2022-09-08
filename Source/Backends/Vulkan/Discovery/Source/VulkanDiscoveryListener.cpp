#include <Backends/Vulkan/VulkanDiscoveryListener.h>
#include <Backends/Vulkan/Layer.h>

// Common
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
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GBV.json";

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

bool InstallImplicitLayer(HKEY key, const wchar_t* path) {
    // Determine the json path
    std::filesystem::path modulePath    = GetCurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GBV.json";

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
    std::filesystem::path layerJsonPath = modulePath / "VK_LAYER_GPUOPEN_GBV.json";

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

bool VulkanDiscoveryListener::Start() {
    // No need to start if already attached
    if (isGlobal) {
        return true;
    }

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
    // Global listener is attached, cannot stop
    if (isGlobal) {
        return false;
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

bool VulkanDiscoveryListener::IsGloballyInstalled() {
    return isGlobal;
}

bool VulkanDiscoveryListener::IsRunning() {
    bool isRunning = false;
    isRunning |= QueryImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    isRunning |= QueryImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
    return isRunning;
}
