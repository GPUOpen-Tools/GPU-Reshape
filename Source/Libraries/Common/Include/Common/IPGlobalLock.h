#pragma once

// System
#ifdef _WIN64
#include <winsock2.h>
#include <sddl.h>
#include <sysinfoapi.h>
#include <Windows.h>
#endif // _WIN64

class IPGlobalLock {
public:
    ~IPGlobalLock() {
#ifdef _WIN64
        CloseHandle(mutexHandle);
#endif // _WIN64
    }

    /// Acquire this global lock
    /// \param inheritHandle system handle inheriting
    /// \return false if already acquired
    bool Acquire(const char* name, bool inheritHandle) {
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
        mutexHandle = CreateMutex(&SecAttr, inheritHandle, name);
        if (mutexHandle && GetLastError() != ERROR_ALREADY_EXISTS) {
            return true;
        }

        // Couldn't open, or already exists
        mutexHandle = nullptr;
        return false;
#else // _WIN64
#    error Not implemented
#endif // _WIN64
    }

private:
#ifdef _WIN64
    HANDLE mutexHandle{};
#endif // _WIN64
};
