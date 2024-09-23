// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Services/HostResolver/HostResolverService.h>
#include <Services/HostResolver/Shared.h>

// Common
#include <Common/IPGlobalLock.h>
#include <Common/FileSystem.h>
#include <Common/System.h>

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
    std::filesystem::path path = GetBaseModuleDirectory() / "GRS.Services.HostResolver.Standalone";

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
