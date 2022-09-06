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
        KEY_WRITE, nullptr,
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
    // Open properties
    HKEY keyHandle{};

    // Open the implicit layer key
    DWORD error = RegOpenKeyW(
        key,
        path,
        &keyHandle
    );
    if (error != ERROR_SUCCESS) {
        // Failed to open, key doesn't exist
        return true;
    }

    // Delete the implicit layer key
    error = RegDeleteKeyW(
        key,
        path
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}
#endif

bool VulkanDiscoveryListener::Install() {
    // Attempt to install implicit non-administrator layer
    if (!InstallImplicitLayer(HKEY_CURRENT_USER, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers")) {
        return false;
    }

    // Attempt to install implicit administrator layer, optional success
    InstallImplicitLayer(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");

    // OK
    return true;
}

bool VulkanDiscoveryListener::Uninstall() {
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
