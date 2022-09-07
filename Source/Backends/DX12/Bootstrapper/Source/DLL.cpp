// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>
#include <Common/String.h>

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

/// Greatly simplifies debugging
#define ENABLE_WHITELIST 1

/// Symbol helper (nay, repent! a macro!)
#define SYMBOL(NAME, STR) \
    static constexpr const char* k##NAME = STR; \
    static constexpr const wchar_t* k##NAME##W = L##STR;

/// Symbols
SYMBOL(D3D12ModuleName, "d3d12.dll");
SYMBOL(LayerModuleName, "Backends.DX12.Layer.dll");
SYMBOL(Kernel32ModuleName, "kernel32.dll");

/// Function types
using PFN_LOAD_LIBRARY_A = HMODULE (WINAPI*)(LPCSTR lpLibFileName);
using PFN_LOAD_LIBRARY_W = HMODULE (WINAPI*)(LPCWSTR lpLibFileName);
using PFN_LOAD_LIBRARY_EX_A = HMODULE (WINAPI*)(LPCSTR lpLibFileName, HANDLE, DWORD);
using PFN_LOAD_LIBRARY_EX_W = HMODULE (WINAPI*)(LPCWSTR lpLibFileName, HANDLE, DWORD);

/// Next
PFN_LOAD_LIBRARY_A Kernel32LoadLibraryAOriginal;
PFN_LOAD_LIBRARY_W Kernel32LoadLibraryWOriginal;
PFN_LOAD_LIBRARY_EX_A Kernel32LoadLibraryExAOriginal;
PFN_LOAD_LIBRARY_EX_W Kernel32LoadLibraryExWOriginal;

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
    std::filesystem::path modulePath = GetCurrentModuleDirectory();

    // Add search directory
    AddDllDirectory(modulePath.wstring().c_str());

    // Process path
    std::filesystem::path path = modulePath / "Backends.DX12.Layer.dll";
#if ENABLE_LOGGING
    LogContext{} << invoker << " - Loading layer " << path << " ... ";
#endif // ENABLE_LOGGING

    // User attempting to load d3d12.dll, warranting bootstrapping of the layer
    if (native) {
        LayerModule = LoadLibraryExW(path.wstring().c_str(), nullptr, 0x0);
    } else {
        LayerModule = Kernel32LoadLibraryExWOriginal(path.wstring().c_str(), nullptr, 0x0);
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
    if (lpLibFileName && std::strcmp(lpLibFileName, kD3D12ModuleName) == 0) {
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
    if (lpLibFileName && std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0) {
        BootstrapLayer("HookLoadLibraryW", false);
    }

    // Pass down call chain, preserve error stack
    return Kernel32LoadLibraryWOriginal(lpLibFileName);
}

WINAPI HMODULE HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE handle, DWORD flags) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && std::strcmp(lpLibFileName, kD3D12ModuleName) == 0) {
        BootstrapLayer("HookLoadLibraryExA", false);
    }

    // Pass down call chain, preserve error stack
    return Kernel32LoadLibraryExAOriginal(lpLibFileName, handle, flags);
}

WINAPI HMODULE HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE handle, DWORD flags) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0) {
        BootstrapLayer("HookLoadLibraryExW", false);
    }

    // Pass down call chain, preserve error stack
    return Kernel32LoadLibraryExWOriginal(lpLibFileName, handle, flags);
}

DWORD DeferredInitialization(void*) {
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
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kD3D12ModuleNameW, &d3d12Module)) {
        // ! Call native LoadLibraryW, not detoured
        BootstrapLayer("Entry detected mounted d3d12 module", false);
        return 0;
    }

#if ENABLE_LOGGING
    LogContext{} << "No mount detected, detouring application\n";
#endif // ENABLE_LOGGING

    // OK
    return 0;
}

/// DLL entrypoint
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

#if ENABLE_WHITELIST
    // Get executable filename
    char filename[2048];
    GetModuleFileName(nullptr, filename, sizeof(filename));

    // Whitelist executable
    if (!std::ends_with(filename, "D3D12HelloTriangle.exe")) {
        return TRUE;
    }
#endif // ENABLE_WHITELIST

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
        (void)CreateThread(
            NULL,
            0,
            DeferredInitialization,
            NULL,
            0,
            NULL
        );

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

        // Attach against original address
        Kernel32LoadLibraryExAOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_EX_A>(GetProcAddress(kernel32Module, "LoadLibraryExA"));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExAOriginal), reinterpret_cast<void*>(HookLoadLibraryExA));

        // Attach against original address
        Kernel32LoadLibraryExWOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_EX_W>(GetProcAddress(kernel32Module, "LoadLibraryExW"));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExWOriginal), reinterpret_cast<void*>(HookLoadLibraryExW));

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

        // May not have detoured at all
        if (Kernel32LoadLibraryAOriginal) {
            // Open transaction
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            // Detach from detours
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryAOriginal), reinterpret_cast<void*>(HookLoadLibraryA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryWOriginal), reinterpret_cast<void*>(HookLoadLibraryW));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExAOriginal), reinterpret_cast<void*>(HookLoadLibraryExA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExWOriginal), reinterpret_cast<void*>(HookLoadLibraryExW));

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
