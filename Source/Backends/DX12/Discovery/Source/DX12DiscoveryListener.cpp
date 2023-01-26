#include <Backends/DX12/DX12DiscoveryListener.h>
#include <Backends/DX12/Shared.h>

// Common
#include <Common/FileSystem.h>
#include <Common/IPGlobalLock.h>

// System
#include <process.h>
#include <tlhelp32.h>

bool QueryService(const wchar_t *name, const wchar_t *path) {
    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the run key
    DWORD error = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0,
        KEY_READ, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Intermediate buffer
    wchar_t buffer[2048];

    // Size of data, replaced
    DWORD bufferSize = sizeof(buffer);

    // Attempt to query key, if it already exists, another service is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        name,
        RRF_RT_REG_SZ,
        nullptr,
        reinterpret_cast<BYTE*>(buffer),
        &bufferSize
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Compare the paths
    return !std::wcscmp(buffer, path);
}

bool InstallService(const wchar_t *name, const wchar_t *path) {
    // Create properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the run key
    DWORD error = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // Intermediate buffer
    wchar_t buffer[2048];

    // Size of data, replaced
    DWORD bufferSize = sizeof(buffer);

    // Attempt to query key, if it already exists, another service is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        name,
        RRF_RT_REG_SZ,
        nullptr,
        reinterpret_cast<BYTE*>(buffer),
        &bufferSize
    );
    if (error == ERROR_SUCCESS) {
        // If already matches, early out
        if (!std::wcscmp(buffer, path)) {
            return true;
        }
    }

    // Not found, so attempt to set the value
    error = RegSetValueExW(
        keyHandle,
        name, 0, REG_SZ,
        reinterpret_cast<const BYTE *>(path), static_cast<uint32_t>((std::wcslen(path) + 1) * sizeof(wchar_t))
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}

bool UninstallService(const wchar_t *name) {
    // Open properties
    HKEY keyHandle{};
    DWORD dwDisposition;

    // Create or open the run key
    DWORD error = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0,
        KEY_ALL_ACCESS, nullptr,
        &keyHandle, &dwDisposition
    );
    if (error != ERROR_SUCCESS) {
        return true;
    }

    // Attempt to query key, if it doesn't exist, no service is present
    error = RegGetValueW(
        keyHandle,
        nullptr,
        name,
        RRF_RT_REG_SZ,
        nullptr,
        nullptr, 0
    );
    if (error != ERROR_SUCCESS) {
        return true;
    }

    // Delete the service key
    error = RegDeleteValueW(
        keyHandle,
        name
    );
    if (error != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return true;
}

DX12DiscoveryListener::DX12DiscoveryListener() {
    // Determine the service path
    servicePath = GetCurrentExecutableDirectory() / "Backends.DX12.Service.exe";

    // Get current status
    isGlobal |= QueryService(L"GPUOpenDX12Service", servicePath.wstring().c_str());
}

bool DX12DiscoveryListener::InstallGlobal() {
    // Install startup keys
    if (!InstallService(L"GPUOpenDX12Service", servicePath.wstring().c_str())) {
        return false;
    }

    // Already started?
    if (IPGlobalLock{}.Acquire(kSharedD3D12ServiceMutexName, false)) {
        if (!StartProcess()) {
            return false;
        }
    }

    // Attached!
    isGlobal = true;

    // OK
    return true;
}

bool DX12DiscoveryListener::UninstallGlobal() {
    // Uninstall startup keys
    if (!UninstallService(L"GPUOpenDX12Service")) {
        return false;
    }

    // Stop any running instance
    if (!StopProcess()) {
        return false;
    }

    // No longer attached
    isGlobal = false;

    // OK
    return true;
}

bool DX12DiscoveryListener::Start() {
    // No need to start if already attached
    if (isGlobal) {
        return true;
    }

    // Already started?
    if (IPGlobalLock{}.Acquire(kSharedD3D12ServiceMutexName, false)) {
        if (!StartProcess()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DX12DiscoveryListener::Stop() {
    // Global listener is attached, cannot stop
    if (isGlobal) {
        return false;
    }

    // Attempt to stop
    return StopProcess();
}

bool DX12DiscoveryListener::StartProcess() {
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
        servicePath.string().data(),
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

bool DX12DiscoveryListener::StopProcess() {
    // Create snapshot
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Entry information
    PROCESSENTRY32 entry{sizeof(PROCESSENTRY32)};

    // Now walk the snapshot of processes
    for (bool result = Process32First(snapshot, &entry); result; result = Process32Next(snapshot, &entry)) {
        if (!std::strcmp(entry.szExeFile, "Backends.DX12.Service.exe")) {
            // Attempt to open the process of interest at PID
            HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (processHandle == nullptr) {
                CloseHandle(snapshot);
                return false;
            }

            // Attempt to terminate
            if (!TerminateProcess(processHandle, 0u)) {
                CloseHandle(snapshot);
                CloseHandle(processHandle);
                return false;
            }

            // Cleanup
            CloseHandle(processHandle);
        }
    }

    // Cleanup
    CloseHandle(snapshot);

    // OK
    return true;
}

bool DX12DiscoveryListener::IsGloballyInstalled() {
    return isGlobal;
}

bool DX12DiscoveryListener::IsRunning() {
    // Create snapshot
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Entry information
    PROCESSENTRY32 entry{sizeof(PROCESSENTRY32)};

    // Now walk the snapshot of processes
    for (bool result = Process32First(snapshot, &entry); result; result = Process32Next(snapshot, &entry)) {
        if (!std::strcmp(entry.szExeFile, "Backends.DX12.Service.exe")) {
            CloseHandle(snapshot);
            return true;
        }
    }

    // Cleanup
    CloseHandle(snapshot);

    // OK
    return false;
}
