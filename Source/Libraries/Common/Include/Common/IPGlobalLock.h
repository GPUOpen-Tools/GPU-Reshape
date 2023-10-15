// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
        if (mutexHandle) {
            return GetLastError() != ERROR_ALREADY_EXISTS;
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
