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
#include <Psapi.h>

// Std
#include <cstring>
#include <mutex>
#include <fstream>
#include <cwchar>
#include <set>

// Detour
#include <Detour/detours.h>

/// Enables naive debugging
#ifndef NDEBUG
#define ENABLE_LOGGING 0
#else // NDEBUG
#define ENABLE_LOGGING 0
#endif // NDEBUG

/// Greatly simplifies debugging
#define ENABLE_WHITELIST 0

/// All whitelisted applications
#if ENABLE_WHITELIST
const char* kWhitelist[] =
{
    /// Hosting service
    /// Must always be included for GPAs
    "GRS.Backends.DX12.Service.exe",

    /// Applications
    /** poof */
};
#endif // ENABLE_WHITELIST

/// Symbol helper (nay, repent! a macro!)
#define SYMBOL(NAME, STR) \
    [[maybe_unused]] static constexpr const char* k##NAME = STR; \
    [[maybe_unused]] static constexpr const wchar_t* k##NAME##W = L##STR;

/// Symbols
SYMBOL(D3D12ModuleName, "d3d12.dll");
SYMBOL(D3D11ModuleName, "d3d11.dll");
SYMBOL(DXGIModuleName, "dxgi.dll");
SYMBOL(AMDAGSModuleName, "amd_ags_x64.dll");
SYMBOL(LayerModuleName, "GRS.Backends.DX12.Layer.dll");
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

/// Shared data segment
#pragma data_seg (".GOD3D12LB")
bool IsBootstrappedAcrossProcess = false;
#pragma data_seg()

/// Linking
#pragma comment(linker, "/Section:.GOD3D12LB,RW")

/// Is this handle the owning instance?
bool IsOwningBootstrapper;

/// Bootstrapped layer
HMODULE LayerModule;
HMODULE D3D12Module;
HMODULE D3D11Module;
HMODULE DXGIModule;
HMODULE AMDAGSModule;

/// Layer function table
D3D12GPUOpenFunctionTable LayerFunctionTable;

/// Module lock
/// Module events are already in sync, however, that's only down the call chain
std::recursive_mutex moduleLock;

/// Snapshot of a module set
using ModuleSnapshot = std::set<HMODULE>;

/// Well documented image base
extern "C" IMAGE_DOS_HEADER __ImageBase;

/// Prototypes
void DetourAMDAGSModule(HMODULE handle, bool insideTransaction);
void DetourD3D11Module(HMODULE handle, bool insideTransaction); 
void DetourD3D12Module(HMODULE handle, bool insideTransaction); 
void DetourDXGIModule(HMODULE handle, bool insideTransaction);
void CommitFunctionTable();

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

bool BootstrapCheckLibrary(HMODULE& handle, const wchar_t* name, bool native) {
    // Early out if already loaded
    if (handle) {
        return false;
    }

    // Get actual module handle
    if (native) {
        handle = LoadLibraryExW(name, nullptr, 0x0);
    } else {
        handle = Kernel32LoadLibraryExWOriginal(name, nullptr, 0x0);
    }

    // Check
    return handle != nullptr;
}

void CommitFunctionTable() {
    // Sanity check
    if (!LayerModule) {
        return;
    }

    // Set function table in layer
    auto* gpaSetFunctionTable = reinterpret_cast<PFN_D3D12_SET_FUNCTION_TABLE_GPUOPEN>(Kernel32GetProcAddressOriginal(LayerModule, "D3D12SetFunctionTableGPUOpen"));
    if (!gpaSetFunctionTable || FAILED(gpaSetFunctionTable(&DetourFunctionTable))) {
#if ENABLE_LOGGING
        LogContext{} << "Failed to set layer function table\n";
#endif // ENABLE_LOGGING
    }
}

void ConditionallyBeginDetour(bool insideTransaction) {
    if (insideTransaction) {
        return;
    }

    // Begin
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
}

void ConditionallyEndDetour(bool insideTransaction) {
    // Commit if needed
    if (!insideTransaction) {
        if (FAILED(DetourTransactionCommit())) {
            return;
        }
    }

    // May be loaded after the bootstrapper has initialized, update the function table if needed
    CommitFunctionTable();
}

void LazyLoadDependentLibraries(bool native) {
    // Begin batch
    ConditionallyBeginDetour(false);

    // D3D12
    if (BootstrapCheckLibrary(D3D12Module, kD3D12ModuleNameW, native)) {
        DetourD3D12Module(D3D12Module, true);
    }

    // D3D11
    if (BootstrapCheckLibrary(D3D11Module, kD3D11ModuleNameW, native)) {
        DetourD3D11Module(D3D11Module, true);
    }

    // DXGI
    if (BootstrapCheckLibrary(DXGIModule, kDXGIModuleNameW, native)) {
        DetourDXGIModule(DXGIModule, true);
    }

    // AGS
    if (BootstrapCheckLibrary(AMDAGSModule, kAMDAGSModuleNameW, native)) {
        DetourAMDAGSModule(DXGIModule, true);
    }

    // End batch
    ConditionallyEndDetour(false);
}

void BootstrapLayer(const char* invoker) {
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
    std::filesystem::path path = modulePath / "GRS.Backends.DX12.Layer.dll";
#if ENABLE_LOGGING
    LogContext{} << invoker << " - Loading layer " << path << " ... ";
#endif // ENABLE_LOGGING

    // Get current session dir
    std::filesystem::path sessionDir = GetCurrentModuleDirectory();

    // Create unique name
    std::string sessionName = "GRS.Backends.DX12.Layer " + GlobalUID::New().ToString() + ".dll";

    // Copy the bootstrapper to a new session, makes handling unique sessions somewhat bearable (certain programs refuse to let go of handle)
    std::filesystem::path sessionPath = sessionDir / sessionName;

    // Copy current bootstrapper
    std::filesystem::copy(path, sessionPath);

    // User attempting to load instrumentable object, warranting bootstrapping of the layer
    LayerModule = Kernel32LoadLibraryExWOriginal(sessionPath.wstring().c_str(), nullptr, 0x0);

    // Fetch function table
    if (LayerModule) {
        // Get hook points
        LayerFunctionTable.next_D3D12CreateDeviceOriginal  = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(LayerModule, "HookID3D12CreateDevice"));
        LayerFunctionTable.next_CreateDXGIFactoryOriginal  = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory"));
        LayerFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory1"));
        LayerFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(Kernel32GetProcAddressOriginal(LayerModule, "HookCreateDXGIFactory2"));
        LayerFunctionTable.next_EnableExperimentalFeatures = reinterpret_cast<PFN_ENABLE_EXPERIMENTAL_FEATURES>(Kernel32GetProcAddressOriginal(LayerModule, "HookD3D12EnableExperimentalFeatures"));
        LayerFunctionTable.next_AMDAGSCreateDevice         = reinterpret_cast<PFN_AMD_AGS_CREATE_DEVICE>(GetProcAddress(LayerModule, "HookAMDAGSCreateDevice"));
        LayerFunctionTable.next_AMDAGSDestroyDevice        = reinterpret_cast<PFN_AMD_AGS_DESTRIY_DEVICE>(GetProcAddress(LayerModule, "HookAMDAGSDestroyDevice"));
        LayerFunctionTable.next_AMDAGSPushMarker           = reinterpret_cast<PFN_AMD_AGS_PUSH_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSPushMarker"));
        LayerFunctionTable.next_AMDAGSPopMarker            = reinterpret_cast<PFN_AMD_AGS_POP_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSPopMarker"));
        LayerFunctionTable.next_AMDAGSSetMarker            = reinterpret_cast<PFN_AMD_AGS_SET_MARKER>(GetProcAddress(LayerModule, "HookAMDAGSSetMarker"));

        // Wrappers
        LayerFunctionTable.next_D3D11On12CreateDeviceOriginal = reinterpret_cast<PFN_D3D11_ON_12_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(LayerModule, "HookD3D11On12CreateDevice"));

        // Initial commit
        CommitFunctionTable();
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

void D3D12GetGPUOpenBootstrapperInfo(D3D12GPUOpenBootstrapperInfo* out) {
    out->version = 1;
}

FARPROC WINAPI HookGetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    // Special name
    if (HIWORD(lpProcName) && !std::strcmp(lpProcName, "D3D12GetGPUOpenBootstrapperInfo")) {
        return reinterpret_cast<FARPROC>(&D3D12GetGPUOpenBootstrapperInfo);
    }

    // Query against layer?
    // ! Disabled for the time being, warrants investigation
#if 0
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
                } else if (!std::strcmp(lpProcName, "D3D12EnableExperimentalFeatures")) {
                    return Kernel32GetProcAddressOriginal(LayerModule, "HookD3D12EnableExperimentalFeatures");
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
#endif // 0

    // Pass down callchain
    return Kernel32GetProcAddressOriginal(hModule, lpProcName);
}

void TryLoadEmbeddedModules(HMODULE handle) {
    // Get the base name
    char baseName[1024];
    if (!GetModuleBaseNameA(GetCurrentProcess(), handle, baseName, sizeof(baseName))) {
        return;
    }

    // Is AGS?
    if (!DetourFunctionTable.next_AMDAGSCreateDevice && Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_CreateDevice") != nullptr) {
        DetourAMDAGSModule(handle, false);
    }

    // Is D3D12?
    if (!DetourFunctionTable.next_D3D12CreateDeviceOriginal && !std::strcmp(baseName, kD3D12ModuleName) && Kernel32GetProcAddressOriginal(handle, "D3D12CreateDevice") != nullptr) {
        DetourD3D12Module(handle, false);
    }

    // Is DXGI?
    if (!DetourFunctionTable.next_CreateDXGIFactoryOriginal && !std::strcmp(baseName, kDXGIModuleName) && Kernel32GetProcAddressOriginal(handle, "CreateDXGIFactory") != nullptr) {
        DetourDXGIModule(handle, false);
    }

    // Is D3D11?
    if (!DetourFunctionTable.next_D3D11On12CreateDeviceOriginal && !std::strcmp(baseName, kD3D11ModuleName) && Kernel32GetProcAddressOriginal(handle, "D3D11On12CreateDevice") != nullptr) {
        DetourD3D11Module(handle, false);
    }
}

ModuleSnapshot GetModuleSnapshot() {
    // Get the process
    HANDLE process = GetCurrentProcess();

    // Determine needed byte count
    DWORD needed{0};
    if (!EnumProcessModules(process, nullptr, 0, &needed)) {
        return {};
    }

    // Get all modules
    std::vector<HMODULE> modules(needed / sizeof(HMODULE), nullptr);
    if (!EnumProcessModules(process, modules.data(), static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &needed)) {
        return {};
    }

    // Create snapshot from module set
    ModuleSnapshot snapshot;
    for (size_t i = 0; i < std::min(needed / sizeof(HMODULE), modules.size()); i++) {
        snapshot.insert(modules[i]);
    }

    // OK
    return snapshot;
}

void DetourForeignModules(const ModuleSnapshot& before) {
    // Get post snapshot
    ModuleSnapshot modules = GetModuleSnapshot();

    // Check all post modules
    for (HMODULE module : modules) {
        if (!module) {
            continue;
        }

        // If part of the pre snapshot, ignore
        if (before.contains(module)) {
            continue;
        }

        // New module, load all embedded proc's
        TryLoadEmbeddedModules(module);
    }
}

HMODULE WINAPI HookLoadLibraryA(LPCSTR lpLibFileName) {
    std::lock_guard guard(moduleLock);
    
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();
    
    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::strcmp(lpLibFileName, kD3D12ModuleName) == 0 || std::strcmp(lpLibFileName, kD3D11ModuleName) == 0 || std::strcmp(lpLibFileName, kDXGIModuleName) == 0)) {
        BootstrapLayer("HookLoadLibraryA");
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryAOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    DetourForeignModules(snapshot);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName) {
    std::lock_guard guard(moduleLock);

#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kD3D11ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kDXGIModuleNameW) == 0)) {
        BootstrapLayer("HookLoadLibraryW");
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryWOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    DetourForeignModules(snapshot);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE handle, DWORD flags) {
    std::lock_guard guard(moduleLock);

#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::strcmp(lpLibFileName, kD3D12ModuleName) == 0 || std::strcmp(lpLibFileName, kD3D11ModuleName) == 0 || std::strcmp(lpLibFileName, kDXGIModuleName) == 0)) {
        BootstrapLayer("HookLoadLibraryExA");
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExAOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    DetourForeignModules(snapshot);

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE handle, DWORD flags) {
    std::lock_guard guard(moduleLock);

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Intercepted library?
    // TODO: May not just be the module name!
    if (lpLibFileName && (std::wcscmp(lpLibFileName, kD3D12ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kD3D11ModuleNameW) == 0 || std::wcscmp(lpLibFileName, kDXGIModuleNameW) == 0)) {
        BootstrapLayer("HookLoadLibraryExW");
    }

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExWOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    DetourForeignModules(snapshot);

    // OK
    return module;
}

DWORD DeferredInitialization(void*) {
    std::lock_guard guard(moduleLock);
    
    // Logging initialization
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

    LogContext{} << "Function table:\n" << std::hex
                 << "LoadLibraryA: 0x" << reinterpret_cast<void *>(HookLoadLibraryA) << " -> 0x" << reinterpret_cast<uint64_t>(Kernel32LoadLibraryAOriginal) << "\n"
                 << "LoadLibraryW: 0x" << reinterpret_cast<void *>(HookLoadLibraryW) << " -> 0x" << reinterpret_cast<uint64_t>(Kernel32LoadLibraryWOriginal) << "\n"
                 << "LoadLibraryExA: 0x" << reinterpret_cast<void *>(HookLoadLibraryExA) << " -> 0x" << reinterpret_cast<uint64_t>(Kernel32LoadLibraryExAOriginal) << "\n"
                 << "LoadLibraryExW: 0x" << reinterpret_cast<void *>(HookLoadLibraryExW) << " -> 0x" << reinterpret_cast<uint64_t>(Kernel32LoadLibraryExWOriginal) << "\n"
                 << "GetProcAddress: 0x" << reinterpret_cast<void *>(HookGetProcAddress) << " -> 0x" << reinterpret_cast<uint64_t>(Kernel32GetProcAddressOriginal) << "\n";
#endif // ENABLE_LOGGING

    // Attempt to find module, directly load the layer if available
    //  i.e. Already loaded or scheduled to be
    if (DXGIModule || D3D12Module || D3D11Module || AMDAGSModule) {
        // Initial snapshot
        ModuleSnapshot snapshot = GetModuleSnapshot();
        
        // ! Call native LoadLibraryW, not detoured
        BootstrapLayer("Entry detected mounted d3d12 module");

        // Query embedded hooks
        DetourForeignModules(snapshot);
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

HRESULT HookD3D11On12CreateDevice(
    IUnknown *pDevice,
    UINT Flags,
    const D3D_FEATURE_LEVEL *pFeatureLevels,
    UINT FeatureLevels,
    IUnknown *const *ppCommandQueues,
    UINT NumQueues,
    UINT NodeMask,
    ID3D11Device **ppDevice,
    ID3D11DeviceContext **ppImmediateContext,
    D3D_FEATURE_LEVEL *pChosenFeatureLevel
    ) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_D3D11On12CreateDeviceOriginal(
        pDevice,
        Flags,
        pFeatureLevels,
        FeatureLevels,
        ppCommandQueues,
        NumQueues,
        NodeMask,
        ppDevice,
        ppImmediateContext,
        pChosenFeatureLevel
    );
}

HRESULT HookD3D12EnableExperimentalFeatures(UINT NumFeatures, const IID *riid, void *pConfigurationStructs, UINT *pConfigurationStructSizes) {
    WaitForDeferredInitialization();
    return LayerFunctionTable.next_EnableExperimentalFeatures(NumFeatures, riid, pConfigurationStructs, pConfigurationStructSizes);
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

void OnDetourModule(HMODULE& module, HMODULE handle) {
    ASSERT(module == nullptr, "Re-entrant detouring");
    module = handle;
}

void DetourAMDAGSModule(HMODULE handle, bool insideTransaction) {
    OnDetourModule(AMDAGSModule, handle);
    
    // Open transaction if needed
    ConditionallyBeginDetour(insideTransaction);

    // Attach against original address
    DetourFunctionTable.next_AMDAGSCreateDevice = reinterpret_cast<PFN_AMD_AGS_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSCreateDevice), reinterpret_cast<void*>(HookAMDAGSCreateDevice));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSDestroyDevice = reinterpret_cast<PFN_AMD_AGS_DESTRIY_DEVICE>(Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_DestroyDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSDestroyDevice), reinterpret_cast<void*>(HookAMDAGSDestroyDevice));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSPushMarker = reinterpret_cast<PFN_AMD_AGS_PUSH_MARKER>(Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_PushMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPushMarker), reinterpret_cast<void*>(HookAMDAGSPushMarker));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSPopMarker = reinterpret_cast<PFN_AMD_AGS_POP_MARKER>(Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_PopMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSPopMarker), reinterpret_cast<void*>(HookAMDAGSPopMarker));

    // Attach against original address
    DetourFunctionTable.next_AMDAGSSetMarker = reinterpret_cast<PFN_AMD_AGS_SET_MARKER>(Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_SetMarker"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_AMDAGSSetMarker), reinterpret_cast<void*>(HookAMDAGSSetMarker));

    // End and update
    ConditionallyEndDetour(insideTransaction);
}

void DetourD3D12Module(HMODULE handle, bool insideTransaction) {
    OnDetourModule(D3D12Module, handle);
    
    // Open transaction if needed
    ConditionallyBeginDetour(insideTransaction);

    // Attach against original address
    DetourFunctionTable.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(handle, "D3D12CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));

    // Attach against original address
    DetourFunctionTable.next_EnableExperimentalFeatures = reinterpret_cast<PFN_ENABLE_EXPERIMENTAL_FEATURES>(Kernel32GetProcAddressOriginal(handle, "D3D12EnableExperimentalFeatures"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_EnableExperimentalFeatures), reinterpret_cast<void*>(HookD3D12EnableExperimentalFeatures));

    // End and update
    ConditionallyEndDetour(insideTransaction);
}

void DetourD3D11Module(HMODULE handle, bool insideTransaction) {
    OnDetourModule(D3D11Module, handle);
    
    // Open transaction if needed
    ConditionallyBeginDetour(insideTransaction);

    // Attach against original address
    DetourFunctionTable.next_D3D11On12CreateDeviceOriginal = reinterpret_cast<PFN_D3D11_ON_12_CREATE_DEVICE>(Kernel32GetProcAddressOriginal(handle, "D3D11On12CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D11On12CreateDeviceOriginal), reinterpret_cast<void*>(HookD3D11On12CreateDevice));

    // End and update
    ConditionallyEndDetour(insideTransaction);
}

void DetourDXGIModule(HMODULE handle, bool insideTransaction) {
    OnDetourModule(DXGIModule, handle);
    
    // Open transaction if needed
    ConditionallyBeginDetour(insideTransaction);

    // Attach against original address
    DetourFunctionTable.next_CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(Kernel32GetProcAddressOriginal(handle, "CreateDXGIFactory"));
    DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));

    // Attach against original address
    DetourFunctionTable.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(Kernel32GetProcAddressOriginal(handle, "CreateDXGIFactory1"));
    if (DetourFunctionTable.next_CreateDXGIFactory1Original) {
        DetourAttach(&reinterpret_cast<void*&>(DetourFunctionTable.next_CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
    }

    // Attach against original address
    DetourFunctionTable.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(Kernel32GetProcAddressOriginal(handle, "CreateDXGIFactory2"));
    if (DetourFunctionTable.next_CreateDXGIFactory2Original) {
        DetourAttach(&reinterpret_cast<void *&>(DetourFunctionTable.next_CreateDXGIFactory2Original), reinterpret_cast<void *>(HookCreateDXGIFactory2));
    }

    // End and update
    ConditionallyEndDetour(insideTransaction);
}

void DetachInitialCreation() {
    // Remove device
    if (DetourFunctionTable.next_D3D12CreateDeviceOriginal) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
        DetourFunctionTable.next_D3D12CreateDeviceOriginal = nullptr;
    }
    
    // Remove wrapper
    if (DetourFunctionTable.next_D3D11On12CreateDeviceOriginal) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_D3D11On12CreateDeviceOriginal), reinterpret_cast<void*>(HookD3D11On12CreateDevice));
        DetourFunctionTable.next_D3D11On12CreateDeviceOriginal = nullptr;
    }

    // Remove device
    if (DetourFunctionTable.next_EnableExperimentalFeatures) {
        DetourDetach(&reinterpret_cast<void*&>(DetourFunctionTable.next_EnableExperimentalFeatures), reinterpret_cast<void*>(HookD3D12EnableExperimentalFeatures));
        DetourFunctionTable.next_EnableExperimentalFeatures = nullptr;
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

/// Pin this module
static void PinBootstrapper() {
    wchar_t buffer[FILENAME_MAX]{0};

    // Get module name of current image
    GetModuleFileNameW((HINSTANCE)&__ImageBase, buffer, FILENAME_MAX);

    // Failure is realistically fatal, but let it continue
    if (!buffer[0]) {
        return;
    }

    // Pin module
    HMODULE ignore;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, buffer, &ignore);
}

/// DLL entrypoint
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    // If this is not the owning bootstrapper, and it is currently bootstrapped elsewhere, report OK
    if (!IsOwningBootstrapper && IsBootstrappedAcrossProcess) {
        return TRUE;
    }

#if ENABLE_WHITELIST
    bool isWhitelisted = false;

    // Check if any of the target applications
    for (const char* name : kWhitelist) {
        if (std::ends_with(GetCurrentExecutableName(), name)) {
            isWhitelisted = true;
        }
    }

    // Not whitelisted?
    if (!isWhitelisted) {
        return FALSE;
    }
#endif // ENABLE_WHITELIST

    // Attach?
    if (dwReason == DLL_PROCESS_ATTACH) {
        // Flag that the bootstrapper is active
        IsBootstrappedAcrossProcess = true;

        // This dll is now the effective owner
        IsOwningBootstrapper = true;

        // Ensure the bootstrapper stays pinned in the process, re-entrant bootstrapping is a mess
        PinBootstrapper();

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

        // Attach against original address
        Kernel32GetProcAddressOriginal = reinterpret_cast<PFN_GET_PROC_ADDRESS>(GetProcAddress(kernel32Module, "GetProcAddress"));
        DetourAttach(&reinterpret_cast<void *&>(Kernel32GetProcAddressOriginal), reinterpret_cast<void *>(HookGetProcAddress));

        // Attempt to create initial detours
        DetourForeignModules(ModuleSnapshot {});

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

            // Unload if attached
            if (D3D12Module) {
                FreeLibrary(D3D12Module);
            }
            if (D3D11Module) {
                FreeLibrary(D3D11Module);
            }
            if (DXGIModule) {
                FreeLibrary(DXGIModule);
            }
            if (AMDAGSModule) {
                FreeLibrary(AMDAGSModule);
            }

            // Flag that the bootstrapper is inactive
            IsBootstrappedAcrossProcess = false;
        }
    }

    // OK
    return TRUE;
}

extern "C" __declspec(dllexport) LRESULT WinHookAttach(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
