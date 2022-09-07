#include <Backends/DX12/DX12DiscoveryListener.h>
#include <Backends/DX12/Shared.h>

// Common
#include <Common/FileSystem.h>
#include <Common/IPGlobalLock.h>

bool InstallService(const wchar_t* name, const wchar_t* path) {
    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the run key
    DWORD error = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0,
        KEY_WRITE, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Attempt to query key, if it already exists, another service is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        name,
        RRF_RT_DWORD,
        nullptr,
        nullptr, 0
    );
    if (error == ERROR_SUCCESS) {
        return true;
    }

    // Not found, so attempt to set the value
    error = RegSetValueExW(
        keyHandle,
        name, 0, REG_SZ,
        reinterpret_cast<const BYTE *>(&path), (std::wcslen(path) + 1) * sizeof(wchar_t)
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}

bool UninstallService(const wchar_t* name) {
    // Open properties
    HKEY keyHandle{};

    // Open the implicit layer key
    DWORD error = RegOpenKeyW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        &keyHandle
    );
    if (error != ERROR_SUCCESS) {
        // Failed to open, key doesn't exist
        return true;
    }

    // Attempt to query key, if it doesn't exist, no service is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        name,
        RRF_RT_DWORD,
        nullptr,
        nullptr, 0
    );
    if (error != ERROR_SUCCESS) {
        return true;
    }

    // Delete the service key
    error = RegDeleteKeyW(
        keyHandle,
        name
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}

bool DX12DiscoveryListener::Install() {
    // Determine the service path
    std::filesystem::path servicePath  = GetCurrentExecutableDirectory() / "Backends.DX12.Service.exe";

    // Install startup keys
    if (!InstallService(L"GPUOpenDX12Service", servicePath.wstring().c_str())) {
        return false;
    }

    // Already started?
    if (!IPGlobalLock{}.Acquire(kSharedMutexName, false)) {
        return true;
    }

    // OK
    return StartProcess();
}

bool DX12DiscoveryListener::Uninstall() {
    // Determine the service path
    std::filesystem::path servicePath  = GetCurrentExecutableDirectory() / "Backends.DX12.Service.exe";

    // Uninstall startup keys
    if (!UninstallService(L"GPUOpenDX12Service")) {
        return false;
    }

    // OK
    return true;
}

bool DX12DiscoveryListener::StartProcess() {
    // Process path
    std::filesystem::path path = GetCurrentModuleDirectory() / "Backends.DX12.Service";

    // Startup info
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));

    // Process info
    PROCESS_INFORMATION processInfo;
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&processInfo, sizeof(processInfo));

    // Attempt to create process
    if (!CreateProcess(
        nullptr,
        path.string().data(),
        nullptr,
        nullptr,
        true,
        DETACHED_PROCESS,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo)) {
        return false;
    }

    // OK
    return true;
}

