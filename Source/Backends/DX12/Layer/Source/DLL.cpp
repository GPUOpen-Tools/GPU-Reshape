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

/// DLL entrypoint
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
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

// TODO: Temporary hack to ensure libs are produced
__declspec(dllexport) int __LibHack;
