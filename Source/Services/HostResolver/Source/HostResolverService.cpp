#include <Services/HostResolver/HostResolverService.h>
#include <Services/HostResolver/Shared.h>

// Common
#include <Common/IPGlobalLock.h>
#include <Common/FileSystem.h>

// System
#ifdef _WIN64
#include <Windows.h>
#endif

bool HostResolverService::Install() {
    // Already acquired?
    if (!IPGlobalLock{}.Acquire(kSharedHostResolverMutexName, false)) {
        return true;
    }

    // Attempt to create process
    return StartProcess();
}

bool HostResolverService::StartProcess() {
    // Process path
    std::filesystem::path path = GetBaseModuleDirectory() / "Services.HostResolver.Standalone";

#ifdef _WIN64
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
#else
#    error Not implemented
#endif
}
