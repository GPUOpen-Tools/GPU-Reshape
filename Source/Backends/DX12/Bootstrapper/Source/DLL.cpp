// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// System
#include <Windows.h>

// Std
#include <mutex>
#include <fstream>
#include <cstring>
#include <cwchar>

// Detour
#include <Detour/detours.h>

/// Enables naive debugging
#define ENABLE_LOGGING 1

/// Symbol helper (nay, repent! a macro!)
#define SYMBOL(NAME, STR) \
    static constexpr const char* k##NAME = STR; \
    static constexpr const wchar_t* k##NAME##W = L##STR;

/// Symbols
SYMBOL(D3D12ModuleName, "d3d12.dll");
SYMBOL(LayerModuleName, "Backends.DX12.Layer.dll");
SYMBOL(Kernel32ModuleName, "kernel32.dll");

/// Function types
using PFN_LOAD_LIBRARY_A = HMODULE (*WINAPI)(LPCSTR lpLibFileName);
using PFN_LOAD_LIBRARY_W = HMODULE (*WINAPI)(LPCWSTR lpLibFileName);

/// Next
PFN_LOAD_LIBRARY_A Kernel32LoadLibraryAOriginal;
PFN_LOAD_LIBRARY_W Kernel32LoadLibraryWOriginal;

/// Bootstrapped layer
HMODULE LayerModule;

#if ENABLE_LOGGING
std::mutex LoggingLock;
std::wofstream LoggingFile;
#endif // ENABLE_LOGGING

#if ENABLE_LOGGING
struct LogContext {
    LogContext() {
        if (LoggingFile.is_open()) {
            LoggingLock.lock();
        }
    }

    ~LogContext() {
        if (LoggingFile.is_open()) {
            LoggingFile.flush();
            LoggingLock.unlock();
        }
    }

    template<typename T>
    LogContext& operator<<(T&& value) {
        if (LoggingFile.is_open()) {
            LoggingFile << value;
        }
        return *this;
    }
};
#endif // ENABLE_LOGGING

void BootstrapLayer(const char* invoker, bool native) {
    // Process path
    std::filesystem::path path = GetCurrentModuleDirectory() / "Backends.DX12.Layer.dll";
#if ENABLE_LOGGING
    LogContext{} << invoker << " - Loading layer " << path << " ... ";
#endif // ENABLE_LOGGING

    // User attempting to load d3d12.dll, warranting bootstrapping of the layer
    if (native) {
        LayerModule = LoadLibraryW(path.wstring().c_str());
    } else {
        LayerModule = Kernel32LoadLibraryWOriginal(path.wstring().c_str());
    }

    // Failed?
#if ENABLE_LOGGING
    if (LayerModule) {
        LogContext{} << "OK\n";
    } else {
        LogContext{} << "Failed [" << GetLastError() << "]\n";
    }
#endif // ENABLE_LOGGING
}

WINAPI HMODULE HookLoadLibraryA(LPCSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && std::strcmp(lpLibFileName, kD3D12ModuleName)) {
        BootstrapLayer("HookLoadLibraryA", false);
    }

    // Pass down call chain, preserve error stack
    return Kernel32LoadLibraryAOriginal(lpLibFileName);
}

WINAPI HMODULE HookLoadLibraryW(LPCWSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && std::wcscmp(lpLibFileName, kD3D12ModuleNameW)) {
        BootstrapLayer("HookLoadLibraryW", false);
    }

    // Pass down call chain, preserve error stack
    return Kernel32LoadLibraryWOriginal(lpLibFileName);
}

DWORD DeferredInitialization(void*) {
    // ! Call native LoadLibraryW, not detoured
    BootstrapLayer("Entry detected mounted d3d12 module", true);

    // OK
    return 0;
}

/// DLL entrypoint
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
#if ENABLE_LOGGING
        // Get executable filename
        char filename[2048];
        GetModuleFileName(nullptr, filename, sizeof(filename));

        // Strip directories
        std::filesystem::path basename = std::filesystem::path(filename).filename().replace_extension("");

        // Setup log path
        std::filesystem::path logPath = GetIntermediatePath("Bootstrapper") / (basename.string() + " " + GlobalUID::New().ToString() + ".txt");

        // Open at path
        LoggingFile.open(logPath);
#endif // ENABLE_LOGGING

        // Attempt to find module, directly load the layer if available
        //  i.e. Already loaded or scheduled to be
        HMODULE d3d12Module{nullptr};
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, kD3D12ModuleNameW, &d3d12Module)) {
            if (!CreateThread(
                NULL,
                0,
                DeferredInitialization,
                NULL,
                0,
                NULL)) {
#if ENABLE_LOGGING
                LogContext{} << "Failed to create initialization thread\n";
#endif // ENABLE_LOGGING
            }
            return TRUE;
        }

#if ENABLE_LOGGING
        LogContext{} << "No mount detected, detouring application\n";
#endif // ENABLE_LOGGING

        // Otherwise, begin detouring against potential loads
        DetourRestoreAfterWith();

        // Open transaction
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        // Attempt to find kernel module
        HMODULE kernel32Module{nullptr};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, kKernel32ModuleNameW, &kernel32Module)) {
            // Nothing we are interested in
            return false;
        }

        // Attach against original address
        Kernel32LoadLibraryAOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_A>(GetProcAddress(kernel32Module, "LoadLibraryA"));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryAOriginal), reinterpret_cast<void*>(HookLoadLibraryA));

        // Attach against original address
        Kernel32LoadLibraryWOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_W>(GetProcAddress(kernel32Module, "LoadLibraryW"));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryWOriginal), reinterpret_cast<void*>(HookLoadLibraryW));

        // Commit all transactions
        if (FAILED(DetourTransactionCommit())) {
            return FALSE;
        }
    }

    // Detach?
    else if (dwReason == DLL_PROCESS_DETACH) {
#if ENABLE_LOGGING
        LogContext{} << "Shutting down\n";

        if (LoggingFile.is_open()) {
            LoggingFile.close();
        }
#endif // ENABLE_LOGGING

        // Release layer
        if (LayerModule) {
            if (!FreeLibrary(LayerModule)) {
                return false;
            }
        }

        // May not have detoured at all
        if (Kernel32LoadLibraryAOriginal) {
            // Open transaction
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            // Detach from detours
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryAOriginal), reinterpret_cast<void*>(HookLoadLibraryA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryWOriginal), reinterpret_cast<void*>(HookLoadLibraryW));

            // Commit all transactions
            if (FAILED(DetourTransactionCommit())) {
                return FALSE;
            }
        }
    }

    // OK
    return TRUE;
}

extern "C" __declspec(dllexport) int WinHookAttach(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
