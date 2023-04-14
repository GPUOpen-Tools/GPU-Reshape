#include <Services/Discovery/NotifyIconDiscoveryListener.h>

// Common
#include <Common/FileSystem.h>

// Std
#include <wchar.h>

// System
#ifdef _WIN32
#   include <Windows.h>
#   include <process.h>
#   include <tlhelp32.h>
#   include <Psapi.h>
#else // _WIN32
#   error Not implemented
#endif // _WIN32

#ifdef _WIN32
inline bool InstallService(const wchar_t *name, const wchar_t *path) {
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

inline bool FindConflictingService(const wchar_t *name, const wchar_t *path) {
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
        return false;
    }

    // Intermediate buffer
    wchar_t buffer[2048];

    // Size of data, replaced
    DWORD bufferSize = sizeof(buffer);

    // Attempt to query key, if it doesn't exist, no service is present
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

    // OK
    return std::wcscmp(buffer, path) != 0;
}

inline bool UninstallService(const wchar_t *name) {
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

inline void StartProcess(char* path) {
    // Startup info
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));

    // Process info
    PROCESS_INFORMATION processInfo;
    startupInfo.cb = sizeof(startupInfo);
    ZeroMemory(&processInfo, sizeof(processInfo));

    // Attempt to create process
    CreateProcess(
        nullptr,
        path,
        nullptr,
        nullptr,
        true,
        DETACHED_PROCESS,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    );
}
#endif // _WIN32

NotifyIconDiscoveryListener::NotifyIconDiscoveryListener() {
    // Determine the service path
    notifyPath = GetCurrentExecutableDirectory() / "GPUReshape.NotifyIcon.exe";

    // Always install startup keys
    InstallService(L"GPUOpenNotifyIconService", notifyPath.wstring().c_str());
}

bool NotifyIconDiscoveryListener::InstallGlobal() {
    // Begin
    StartProcess(notifyPath.string().data());

    // OK
    return true;
}

bool NotifyIconDiscoveryListener::UninstallGlobal() {
    return true;
}

bool NotifyIconDiscoveryListener::HasConflictingInstances() {
    return FindConflictingService(L"GPUOpenNotifyIconService", notifyPath.wstring().c_str());
}

bool NotifyIconDiscoveryListener::UninstallConflictingInstances() {
    // Remove bad service if needed
    if (FindConflictingService(L"GPUOpenNotifyIconService", notifyPath.wstring().c_str()) && !UninstallService(L"GPUOpenNotifyIconService")) {
        return false;
    }

    // OK
    return true;
}

bool NotifyIconDiscoveryListener::Start() {
    // Begin
    StartProcess(notifyPath.string().data());

    // OK
    return true;
}

bool NotifyIconDiscoveryListener::Stop() {
    return true;
}

void NotifyIconDiscoveryListener::SetupBootstrappingEnvironment(const DiscoveryProcessInfo &info, DiscoveryBootstrappingEnvironment &environment) {
    // Nothing
}

bool NotifyIconDiscoveryListener::IsGloballyInstalled() {
    return true;
}

bool NotifyIconDiscoveryListener::IsRunning() {
    return true;
}
