#include <Backends/Vulkan/VulkanDiscoveryListener.h>
#include <Backends/Vulkan/Layer.h>

// Common
#include <Common/Path.h>

// System
#ifdef _WIN32
#   include <Windows.h>
#else
#   error Not implemented
#endif

VulkanDiscoveryListener::~VulkanDiscoveryListener() {
    if (!keyOwner) {
        return;
    }

    // Remove layer
    UninstallImplicitLayer();
}

bool VulkanDiscoveryListener::Install() {
    // Attempt to install implicit layer
    if (!InstallImplicitLayer()) {
        return false;
    }

    // Key is owned
    keyOwner = true;

    // OK
    return true;
}

#ifdef _WIN32
bool VulkanDiscoveryListener::InstallImplicitLayer() {
    // Determine the json path
    std::filesystem::path modulePath    = CurrentExecutableDirectory();
    std::filesystem::path layerJsonPath = modulePath / "VK_GPUOpen_GBV.json";

    // Key name
    key = L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers";

    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the implicit layer key
    DWORD error = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        key.c_str(),
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

void VulkanDiscoveryListener::UninstallImplicitLayer() {
    // ...
}
#endif
