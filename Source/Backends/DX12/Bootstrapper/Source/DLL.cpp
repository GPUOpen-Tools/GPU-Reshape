// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>
#include <Common/String.h>

// Layer (only header)
#include <Backends/DX12/Layer.h>
#include <Backends/DX12/Ordinal.h>

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
SYMBOL(DXGIModuleName, "dxgi.dll");
SYMBOL(AMDAGSModuleName, "amd_ags_x64.dll");
SYMBOL(LayerModuleName, "Backends.DX12.Layer.dll");
SYMBOL(Kernel32ModuleName, "kernel32.dll");

/// Function types
using PFN_GET_PROC_ADDRESS  = FARPROC (WINAPI*)(HMODULE hModule, LPCSTR lpProcName);
using PFN_LOAD_LIBRARY_A    = HMODULE (WINAPI*)(LPCSTR lpLibFileName);
using PFN_LOAD_LIBRARY_W    = HMODULE (WINAPI*)(LPCWSTR lpLibFileName);
using PFN_LOAD_LIBRARY_EX_A = HMODULE (WINAPI*)(LPCSTR lpLibFileName, HANDLE, DWORD);
using PFN_LOAD_LIBRARY_EX_W = HMODULE (WINAPI*)(LPCWSTR lpLibFileName, HANDLE, DWORD);

/// Next
PFN_GET_PROC_ADDRESS      Kernel32GetProcAddressOriginal;
PFN_LOAD_LIBRARY_A        Kernel32LoadLibraryAOriginal;
PFN_LOAD_LIBRARY_W        Kernel32LoadLibraryWOriginal;
PFN_LOAD_LIBRARY_EX_A     Kernel32LoadLibraryExAOriginal;
PFN_LOAD_LIBRARY_EX_W     Kernel32LoadLibraryExWOriginal;
D3D12GPUOpenFunctionTable DetourFunctionTable;

/// Event fired after deferred initialization has completed
HANDLE InitializationEvent;

/// Has the layer attempted initialization prior?
bool HasInitializedOrFailed;

/// Bootstrapped layer
HMODULE LayerModule;
HMODULE D3D12Module;
HMODULE DXGIModule;
HMODULE AMDAGSModule;

/// Layer function table
D3D12GPUOpenFunctionTable LayerFunctionTable;

/// Prototypes
void DetourInitialAMDAGS(HMODULE, bool);

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
    // No re-entry if an attempt has already been made
    if (HasInitializedOrFailed) {
        return;
    }

    // An attempt was made
    HasInitializedOrFailed = true;

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

    // Get actual module handles
    if (native) {
        D3D12Module = LoadLibraryExW(kD3D12ModuleNameW, nullptr, 0x0);
        DXGIModule = LoadLibraryExW(kDXGIModuleNameW, nullptr, 0x0);
        AMDAGSModule = LoadLibraryExW(kAMDAGSModuleNameW, nullptr, 0x0);
    } else {
        D3D12Module = Kernel32LoadLibraryExWOriginal(kD3D12ModuleNameW, nullptr, 0x0);
        DXGIModule = Kernel32LoadLibraryExWOriginal(kDXGIModuleNameW, nullptr, 0x0);
        AMDAGSModule = Kernel32LoadLibraryExWOriginal(kAMDAGSModuleNameW, nullptr, 0x0);
    }

    // Failed?
    // Note: AMDAGSModule is optional, extension service
#if ENABLE_LOGGING
    if (!D3D12Module || !DXGIModule) {
        LogContext{} << "Failed to load " << kD3D12ModuleName << " , " << kDXGIModuleName <<  " [" << GetLastError() << "]\n";
    }
#endif // ENABLE_LOGGING

    // Get device creation if not hooked
    if (!DetourFunctionTable.next_D3D12CreateDeviceOriginal) {
        DetourFunctionTable.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(D3D12Module, "D3D12CreateDevice"));
    }

    // Get factory creation 0 if not hooked
    if (!DetourFunctionTable.next_CreateDXGIFactoryOriginal) {
        DetourFunctionTable.next_CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(DXGIModule, "CreateDXGIFactory"));
    }

    // Get factory creation 1 if not hooked
    if (!DetourFunctionTable.next_CreateDXGIFactory1Original) {
        DetourFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(DXGIModule, "CreateDXGIFactory1"));
    }

    // Get factory creation 2 if not hooked
    if (!DetourFunctionTable.next_CreateDXGIFactory2Original) {
        DetourFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(DXGIModule, "CreateDXGIFactory2"));
    }

    // Get amd ags if not hooked
    if (!DetourFunctionTable.next_AMDAGSCreateDevice) {
        DetourFunctionTable.next_AMDAGSCreateDevice  = reinterpret_cast<PFN_AMD_AGS_CREATE_DEVICE>(GetProcAddress(AMDAGSModule, "agsDriverExtensionsDX12_CreateDevice"));
        DetourFunctionTable.next_AMDAGSDestroyDevice = reinterpret_cast<PFN_AMD_AGS_DESTRIY_DEVICE>(GetProcAddress(AMDAGSModule, "agsDriverExtensionsDX12_DestroyDevice"));
        DetourFunctionTable.next_AMDAGSPushMarker    = reinterpret_cast<PFN_AMD_AGS_PUSH_MARKER>(GetProcAddress(AMDAGSModule, "agsDriverExtensionsDX12_PushMarker"));
        DetourFunctionTable.next_AMDAGSPopMarker     = reinterpret_cast<PFN_AMD_AGS_POP_MARKER>(GetProcAddress(AMDAGSModule, "agsDriverExtensionsDX12_PopMarker"));
        DetourFunctionTable.next_AMDAGSSetMarker     = reinterpret_cast<PFN_AMD_AGS_SET_MARKER>(GetProcAddress(AMDAGSModule, "agsDriverExtensionsDX12_SetMarker"));
    }

    // User attempting to load d3d12.dll, warranting bootstrapping of the layer
    if (native) {
        LayerModule = LoadLibraryExW(sessionPath.wstring().c_str(), nullptr, 0x0);
    } else {
        LayerModule = Kernel32LoadLibraryExWOriginal(sessionPath.wstring().c_str(), nullptr, 0x0);
    }

    // Fetch function table
    if (LayerModule) {
        // Get hook points
        LayerFunctionTable.next_D3D12CreateDeviceOriginal  = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(LayerModule, "HookID3D12CreateDevice"));
        LayerFunctionTable.next_CreateDXGIFactoryOriginal  = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory"));
        LayerFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory1"));
        LayerFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory2"));
        LayerFunctionTable.next_AMDAGSCreateDevice         = reinterpret_cast<PFN_AMD_AGS_CREATE_DEVICE>(GetProcAddress(LayerModule, "HookAMDAGSCreateDevice"));
        LayerFunctionTable.next_AMDAGSDestroyDevice        = reinterpret_cast<PFN_AMD_AGS_DESTRIY_DEVICE>(GetProcAddress(LayerModule, "HookAMDAGSDestroyDevice"));
        LayerFunctionTable.next_AMDAGSPushMarker           = reinterpret_cast<PFN_AMD_AGS_PUSH_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSPushMarker"));
        LayerFunctionTable.next_AMDAGSPopMarker            = reinterpret_cast<PFN_AMD_AGS_POP_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSPopMarker"));
        LayerFunctionTable.next_AMDAGSSetMarker            = reinterpret_cast<PFN_AMD_AGS_SET_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSSetMarker"));

        // Failed?
        // Note: AMDAGSModule is optional, extension service
        if (!LayerFunctionTable.next_D3D12CreateDeviceOriginal ||
            !LayerFunctionTable.next_CreateDXGIFactoryOriginal ||
            !LayerFunctionTable.next_CreateDXGIFactory1Original ||
            !LayerFunctionTable.next_CreateDXGIFactory2Original) {
#if ENABLE_LOGGING
            LogContext{} << "Failed to get layer function table\n";
#endif // ENABLE_LOGGING
        }

        // Set function table in layer
        auto* gpaSetFunctionTable = reinterpret_cast<PFN_D3D12_SET_FUNCTION_TABLE_GPUOPEN>(Kernel32GetProcAddressOriginal(LayerModule, "D3D12SetFunctionTableGPUOpen"));
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

FARPROC WINAPI HookGetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    // Query against layer?
    if (LayerModule) {
        // Non-ordinal?
        if (HIWORD(lpProcName)) {
            // DXGI query?
            if (hModule == DXGIModule) {
                if (!std::strcmp(lpProcName, "CreateDXGIFactory")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory");
                } else if (!std::strcmp(lpProcName, "CreateDXGIFactory1")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory1");
                } else if (!std::strcmp(lpProcName, "CreateDXGIFactory2")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory2");
                }
            }

            // D3D12 query?
            else if (LayerModule == D3D12Module) {
                if (!std::strcmp(lpProcName, "D3D12CreateDevice")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookID3D12CreateDevice");
                }
            }

            // AGS query?
            else if (LayerModule == AMDAGSModule) {
                if (!std::strcmp(lpProcName, "agsDriverExtensionsDX12_CreateDevice")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookAMDAGSCreateDevice");
                } else if (!std::strcmp(lpProcName, "agsDriverExtensionsDX12_DestroyDevice")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookAMDAGSDestroyDevice");
                } else if (!std::strcmp(lpProcName, "agsDriverExtensionsDX12_PushMarker")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookAMDAGSPushMarker");
                } else if (!std::strcmp(lpProcName, "agsDriverExtensionsDX12_PopMarker")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookAMDAGSPopMarker");
                } else if (!std::strcmp(lpProcName, "agsDriverExtensionsDX12_SetMarker")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookAMDAGSSetMarker");
                }
            }
        } else {
            DWORD ordinalIndex = LOWORD(lpProcName);

            // DXGI query?
            if (hModule == DXGIModule) {
                // TODO: DXGI ordinals!
            }

            // D3D12 query?
            if (hModule == D3D12Module) {
                if (ordinalIndex == static_cast<uint32_t>(D3D12Ordinals::D3D12CreateDevice)) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookID3D12CreateDevice");
                }
            }
        }
    }

    // Pass down callchain
    return Kernel32GetProcAddressOriginal(hModule, lpProcName);
}

void TryLoadEmbeddedAMDAGS(HMODULE handle) {
    if (!DetourFunctionTable.next_AMDAGSCreateDevice && GetProcAddress(handle, "agsDriverExtensionsDX12_CreateDevice") != nullptr) {
        DetourInitialAMDAGS(handle, false);
    }
}

HMODULE WINAPI HookLoadLibraryA(LPCSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::strcmp(lpLibFileName, kD3D12ModuleName) == 0 || std::strcmp(lpLibFileName, kDXGIModuleName) == 0)) {
        BootstrapLayer("HookLoadLibraryA", false);
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryAOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    TryLoadEmbeddedAMDAGS(module);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kDXGIModuleNameW) == 0)) {
        BootstrapLayer("HookLoadLibraryW", false);
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryWOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    TryLoadEmbeddedAMDAGS(module);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE handle, DWORD flags) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::strcmp(lpLibFileName, kD3D12ModuleName) == 0 || std::strcmp(lpLibFileName, kDXGIModuleName) == 0)) {
        BootstrapLayer("HookLoadLibraryExA", false);
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExAOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    TryLoadEmbeddedAMDAGS(module);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE handle, DWORD flags) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kDXGIModuleNameW) == 0)) {
        BootstrapLayer("HookLoadLibraryExW", false);
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExWOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    TryLoadEmbeddedAMDAGS(module);

    // OK
    return module;
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
    HMODULE module{nullptr};
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kD3D12ModuleNameW, &module)
        || GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kDXGIModuleNameW, &module)) {
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

AGSReturnCode HookAMDAGSCreateDevice(AGSContext* context, const AGSDX12DeviceCreationParams* creationParams, const AGSDX12ExtensionParams* extensionParams, AGSDX12ReturnedParams* returnedParams) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_AMDAGSCreateDevice(context, creationParams, extensionParams, returnedParams);
}

AGSReturnCode HookAMDAGSDestroyDevice(AGSContext* context, ID3D12Device* device, unsigned int* deviceReferences) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_AMDAGSDestroyDevice(context, device, deviceReferences);
}

AGSReturnCode HookAMDAGSPushMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_AMDAGSPushMarker(context, commandList, data);
}

AGSReturnCode HookAMDAGSPopMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_AMDAGSPopMarker(context, commandList);
}

AGSReturnCode HookAMDAGSSetMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_AMDAGSSetMarker(context, commandList, data);
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

void DetourInitialAMDAGS(HMODULE handle, bool insideTransaction) {
    // Open transaction if needed
    if (!insideTransaction) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
    }

    // Attach against original address
    DetourFunctionTable.next_AMDAGSCreateDevice = reinterpret_cast<PFN_AMD_AGS_CREATE_DEVICE>(GetProcAddress(handle, "agsDriverExtensionsDX12_CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSCreateDevice), reinterpret_cast<void*>(HookAMDAGSCreateDevice));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSDestroyDevice = reinterpret_cast<PFN_AMD_AGS_DESTRIY_DEVICE>(GetProcAddress(handle, "agsDriverExtensionsDX12_DestroyDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSDestroyDevice), reinterpret_cast<void*>(HookAMDAGSDestroyDevice));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSPushMarker = reinterpret_cast<PFN_AMD_AGS_PUSH_MARKER>(GetProcAddress(handle, "agsDriverExtensionsDX12_PushMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPushMarker), reinterpret_cast<void*>(HookAMDAGSPushMarker));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSPopMarker = reinterpret_cast<PFN_AMD_AGS_POP_MARKER>(GetProcAddress(handle, "agsDriverExtensionsDX12_PopMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPopMarker), reinterpret_cast<void*>(HookAMDAGSPopMarker));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSSetMarker = reinterpret_cast<PFN_AMD_AGS_SET_MARKER>(GetProcAddress(handle, "agsDriverExtensionsDX12_SetMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSSetMarker), reinterpret_cast<void*>(HookAMDAGSSetMarker));

    // Commit if needed
    if (!insideTransaction) {
        if (FAILED(DetourTransactionCommit())) {
            return;
        }
    }

    // AGS may be loaded after the bootstrapper has initialized, update the function table if needed
    if (LayerModule) {
        auto* gpaSetFunctionTable = reinterpret_cast<PFN_D3D12_SET_FUNCTION_TABLE_GPUOPEN>(Kernel32GetProcAddressOriginal(LayerModule, "D3D12SetFunctionTableGPUOpen"));
        if (!gpaSetFunctionTable || FAILED(gpaSetFunctionTable(&DetourFunctionTable))) {
#if ENABLE_LOGGING
            LogContext{} << "Failed to update layer function table\n";
#endif // ENABLE_LOGGING
        }
    }
}

void DetourInitialCreation() {
    HMODULE handle = nullptr;

    // Attempt to find d3d12 module
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kD3D12ModuleNameW, &handle)) {
        // Attach against original address
        DetourFunctionTable.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(handle, "D3D12CreateDevice"));
        DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
    }

    // Attempt to find dxgi module
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, kDXGIModuleNameW, &handle)) {
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

    // Attempt to find amd ags module
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, kAMDAGSModuleNameW, &handle)) {
        DetourInitialAMDAGS(handle, true);
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

    // Remove AMD ags
    if (DetourFunctionTable.next_AMDAGSCreateDevice) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSCreateDevice), reinterpret_cast<void*>(HookAMDAGSCreateDevice));
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSDestroyDevice), reinterpret_cast<void*>(HookAMDAGSDestroyDevice));
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPushMarker), reinterpret_cast<void*>(HookAMDAGSPushMarker));
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPopMarker), reinterpret_cast<void*>(HookAMDAGSPopMarker));
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSSetMarker), reinterpret_cast<void*>(HookAMDAGSSetMarker));

        DetourFunctionTable.next_AMDAGSCreateDevice = nullptr;
        DetourFunctionTable.next_AMDAGSDestroyDevice = nullptr;
        DetourFunctionTable.next_AMDAGSPushMarker = nullptr;
        DetourFunctionTable.next_AMDAGSPopMarker = nullptr;
        DetourFunctionTable.next_AMDAGSSetMarker = nullptr;
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

        // Attach against original address
        Kernel32GetProcAddressOriginal = reinterpret_cast<PFN_GET_PROC_ADDRESS>(GetProcAddress(kernel32Module, "GetProcAddress"));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32GetProcAddressOriginal), reinterpret_cast<void*>(HookGetProcAddress));

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
            DetourDetach(&reinterpret_cast<void*&>(Kernel32GetProcAddressOriginal), reinterpret_cast<void*>(HookGetProcAddress));
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
