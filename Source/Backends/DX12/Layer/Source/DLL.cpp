// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// System
#include <Windows.h>

// Layer
#include <Backends/DX12/DXGIFactory.h>
#include <Backends/DX12/Device.h>

// Detour
#include <Detour/detours.h>

/// Device detouring
GlobalDXGIFactoryDetour dxgiFactoryDetour;
GlobalDeviceDetour deviceDetour;

/// Well documented image base
extern "C" IMAGE_DOS_HEADER __ImageBase;

/// Was the bootstrapper attached on DLL_PROCESS_ATTACH?
bool IsBootstrappedOnAttach = false;

/// Check if the process is already bootstrapped, the boostrapper performs its own detouring of calls
/// \return true if bootstrapped
static bool IsBootstrapped() {
    auto* gpaBootstrapperInfo = reinterpret_cast<PFN_D3D12_GET_GPUOPEN_BOOTSTRAPPER_INFO>(GetProcAddress((HINSTANCE)&__ImageBase, "D3D12GetGPUOpenBootstrapperInfo"));
    if (!gpaBootstrapperInfo) {
        return false;
    }

    // Get info
    D3D12GPUOpenBootstrapperInfo info{};
    gpaBootstrapperInfo(&info);

    // Version check
    return info.version >= 1;
}

/// DLL entrypoint
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
        // If the process is already bootstrapped, skip
        if (IsBootstrapped()) {
            IsBootstrappedOnAttach = true;
            return TRUE;
        }

        DetourRestoreAfterWith();

        // Open transaction
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        // Install dxgi factory
        if (!dxgiFactoryDetour.Install()) {
            return FALSE;
        }

        // Install device
        if (!deviceDetour.Install()) {
            return FALSE;
        }

        // Commit all transactions
        if (FAILED(DetourTransactionCommit())) {
            return FALSE;
        }
    }

    // Detach?
    else if (dwReason == DLL_PROCESS_DETACH) {
        // If the process is already bootstrapped, skip
        if (IsBootstrappedOnAttach) {
            return TRUE;
        }

        // Open transaction
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        // Uninstall detours
        dxgiFactoryDetour.Uninstall();
        deviceDetour.Uninstall();

        // Commit all transactions
        if (FAILED(DetourTransactionCommit())) {
            return FALSE;
        }
    }

    // OK
    return TRUE;
}
