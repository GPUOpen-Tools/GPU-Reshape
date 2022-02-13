#pragma once

// Resolver
#include "Shared.h"

// System
#ifdef _WIN64
#include <winsock2.h>
#include <sddl.h>
#include <sysinfoapi.h>
#include <Windows.h>
#endif

class IPGlobalLock {
public:
    ~IPGlobalLock() {
        CloseHandle(mutexHandle);
    }

    /// Acquire this global lock
    /// \param inheritHandle system handle inheriting
    /// \return false if already acquired
    bool Acquire(bool inheritHandle) {
#ifdef _WIN64
        LPCSTR pszStringSecurityDescriptor = "D:(A;;GA;;;WD)(A;;GA;;;AN)S:(ML;;NW;;;ME)";

        // Create security description
        PSECURITY_DESCRIPTOR pSecDesc;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(pszStringSecurityDescriptor, SDDL_REVISION_1, &pSecDesc, nullptr)) {
            return false;
        }

        // Security attributes
        SECURITY_ATTRIBUTES SecAttr;
        SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttr.lpSecurityDescriptor = pSecDesc;
        SecAttr.bInheritHandle = inheritHandle;

        // Try to open and inherit mutex
        mutexHandle = CreateMutex(&SecAttr, inheritHandle, kWin32SharedMutexName);
        if (mutexHandle && GetLastError() != ERROR_ALREADY_EXISTS) {
            return true;
        }

        // Couldn't open, or already exists
        mutexHandle = nullptr;
        return false;
#else
#    error Not implemented
#endif
    }

private:
#ifdef _WIN64
    HANDLE mutexHandle{};
#endif
};
