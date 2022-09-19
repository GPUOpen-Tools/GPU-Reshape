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

/// Check if the process is already bootstrapped, the boostrapper performs its own detouring of calls
/// \return true if bootstrapped
static bool IsBootstrapped() {
    char buffer[32];

    // Load the bootstrapper env flag
    size_t length;
    getenv_s(&length, buffer, "GPUOPEN_DX12_BOOTSTRAPPER");

    // If set to "1", the bootstrapper attached this layer
    return !std::strcmp(buffer, "1");
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
        if (IsBootstrapped()) {
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
