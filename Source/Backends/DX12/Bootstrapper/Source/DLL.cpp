// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>
#include <Common/String.h>

// Layer (only header)
#include <Backends/DX12/Layer.h>

// System
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// Std
#include <mutex>
#include <fstream>
#include <cstring>
#include <cwchar>

// Detour
#include <Detour/detours.h>

/// Enables naive debugging
#ifndef NDEBUG
#define ENABLE_LOGGING 1
#else // NDEBUG
#define ENABLE_LOGGING 0
#endif // NDEBUG

/// Greatly simplifies debugging
#define ENABLE_WHITELIST 0

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
PFN_LOAD_LIBRARY_A        Kernel32LoadLibraryAOriginal;
PFN_LOAD_LIBRARY_W        Kernel32LoadLibraryWOriginal;
PFN_LOAD_LIBRARY_EX_A     Kernel32LoadLibraryExAOriginal;
PFN_LOAD_LIBRARY_EX_W     Kernel32LoadLibraryExWOriginal;
D3D12GPUOpenFunctionTable DetourFunctionTable;

/// Event fired after deferred initialization has completed
HANDLE InitializationEvent;

/// Bootstrapped layer
HMODULE LayerModule;

/// Layer function table
D3D12GPUOpenFunctionTable LayerFunctionTable;

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
    // Get module path, the bootstrapper sessions are hosted under Intermediate
    std::filesystem::path modulePath = GetBaseModuleDirectory();

    // Add search directory
    AddDllDirectory(modulePath.wstring().c_str());

    // Process path
    std::filesystem::path path = modulePath / "Backends.DX12.Layer.dll";
#if ENABLE_LOGGING
    LogContext{} << invoker << " - Loading layer " << path << " ... ";
#endif // ENABLE_LOGGING

    // Get current session dir
    std::filesystem::path sessionDir = GetCurrentModuleDirectory();

    // Create unique name
    std::string sessionName = "Backends.DX12.Layer " + GlobalUID::New().ToString() + ".dll";

    // Copy the bootstrapper to a new session, makes handling unique sessions somewhat bearable (certain programs refuse to let go of handle)
    std::filesystem::path sessionPath = sessionDir / sessionName;

    // Copy current bootstrapper
    std::filesystem::copy(path, sessionPath);

    // Flag that the bootstrapper is active
    _putenv_s("GPUOPEN_DX12_BOOTSTRAPPER", "1");

    // User attempting to load d3d12.dll, warranting bootstrapping of the layer
    if (native) {
        LayerModule = LoadLibraryExW(sessionPath.wstring().c_str(), nullptr, 0x0);
    } else {
        LayerModule = Kernel32LoadLibraryExWOriginal(sessionPath.wstring().c_str(), nullptr, 0x0);
    }

    // Fetch function table
    if (LayerModule) {
        // Get hook points
        LayerFunctionTable.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(LayerModule, "HookID3D12CreateDevice"));
        LayerFunctionTable.next_CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(LayerModule, "HookCreateDXGIFactory"));
        LayerFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(LayerModule, "HookCreateDXGIFactory1"));
        LayerFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(LayerModule, "HookCreateDXGIFactory2"));
        if (!LayerFunctionTable.next_D3D12CreateDeviceOriginal ||
            !LayerFunctionTable.next_CreateDXGIFactoryOriginal ||
            !LayerFunctionTable.next_CreateDXGIFactory1Original ||
            !LayerFunctionTable.next_CreateDXGIFactory2Original) {
#if ENABLE_LOGGING
            LogContext{} << "Failed to get layer function table\n";
#endif // ENABLE_LOGGING
        }

        // Set function table in layer
        auto* gpaSetFunctionTable = reinterpret_cast<PFN_D3D12_SET_FUNCTION_TABLE_GPUOPEN>(GetProcAddress(LayerModule, "D3D12SetFunctionTableGPUOpen"));
        if (!gpaSetFunctionTable || FAILED(gpaSetFunctionTable(&DetourFunctionTable))) {
#if ENABLE_LOGGING
            LogContext{} << "Failed to set layer function table\n";
#endif // ENABLE_LOGGING
        }
    }

    // Failed?
#if ENABLE_LOGGING
    if (LayerModule) {
        LogContext{} << "OK\n";
    } else {
        LogContext{} << "Failed [" << GetLastError() << "]\n";
    }
#endif // ENABLE_LOGGING

    // Fire event
    if (!SetEvent(InitializationEvent))
    {
#if ENABLE_LOGGING
        LogContext{} << "Failed to release deferred initialization lock\n";
#endif // ENABLE_LOGGING
    }
}

HMODULE WINAPI HookLoadLibraryA(LPCSTR lpLibFileName) {
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

HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName) {
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

HMODULE WINAPI HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE handle, DWORD flags) {
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

HMODULE WINAPI HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE handle, DWORD flags) {
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
    std::filesystem::path logPath = GetIntermediatePath("Bootstrapper/Entries") / (basename.string() + " " + GlobalUID::New().ToString() + ".txt");

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

void WaitForDeferredInitialization() {
    // Wait for the deferred event
    DWORD result = WaitForSingleObject(InitializationEvent, INFINITE);
    if (result != WAIT_OBJECT_0) {
#if ENABLE_LOGGING
        LogContext{} << "Failed to wait for deferred initialization\n";
#endif // ENABLE_LOGGIN
        return;
    }
}

HRESULT WINAPI HookID3D12CreateDevice(
    _In_opt_ IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void **ppDevice) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, riid, ppDevice);
}

HRESULT WINAPI HookCreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_CreateDXGIFactoryOriginal(riid, ppFactory);
}

HRESULT WINAPI HookCreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_CreateDXGIFactory1Original(riid, ppFactory);
}

HRESULT WINAPI HookCreateDXGIFactory2(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_CreateDXGIFactory2Original(flags, riid, ppFactory);
}

void DetourInitialCreation() {
    HMODULE handle = nullptr;

    // Attempt to find d3d12 module
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"d3d12.dll", &handle)) {
        // Attach against original address
        DetourFunctionTable.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(handle, "D3D12CreateDevice"));
        DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
    }

    // Attempt to find dxgi module
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"dxgi.dll", &handle)) {
        // Attach against original address
        DetourFunctionTable.next_CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(handle, "CreateDXGIFactory"));
        DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));

        // Attach against original address
        DetourFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(handle, "CreateDXGIFactory1"));
        if (DetourFunctionTable.next_CreateDXGIFactory1Original) {
            DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
        }

        // Attach against original address
        DetourFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(handle, "CreateDXGIFactory2"));
        if (DetourFunctionTable.next_CreateDXGIFactory2Original) {
            DetourAttach(&reinterpret_cast<void *&>(DetourFunctionTable.next_CreateDXGIFactory2Original), reinterpret_cast<void *>(HookCreateDXGIFactory2));
        }
    }
}

void DetachInitialCreation() {
    // Remove device
    if (DetourFunctionTable.next_D3D12CreateDeviceOriginal) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
        DetourFunctionTable.next_D3D12CreateDeviceOriginal = nullptr;
    }

    // Remove factory revision 0
    if (DetourFunctionTable.next_CreateDXGIFactoryOriginal) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));
        DetourFunctionTable.next_CreateDXGIFactoryOriginal = nullptr;
    }

    // Remove factory revision 1
    if (DetourFunctionTable.next_CreateDXGIFactory1Original) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
        DetourFunctionTable.next_CreateDXGIFactory1Original = nullptr;
    }

    // Remove factory revision 2
    if (DetourFunctionTable.next_CreateDXGIFactory2Original) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactory2Original), reinterpret_cast<void*>(HookCreateDXGIFactory2));
        DetourFunctionTable.next_CreateDXGIFactory2Original = nullptr;
    }
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
    if (!std::ends_with(filename, "Backends.DX12.Service.exe") && !std::ends_with(filename, "ModelViewer.exe")) {
        // Note: This is a terrible idea, hook will attempt to load over and over
        return FALSE;
    }
#endif // ENABLE_WHITELIST

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
        // Create deferred initialization event
        InitializationEvent = CreateEvent(nullptr, true, false, nullptr);

        // Defer the initialization, thread only invoked after the dll attach chain
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

        // Attempt to create initial detours
        DetourInitialCreation();

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

            // Detach initial creation
            DetachInitialCreation();

            // Release event
            CloseHandle(InitializationEvent);

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
