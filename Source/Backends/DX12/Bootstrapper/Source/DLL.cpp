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

// Common
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>
#include <Common/String.h>

// Layer (only header)
#include <Backends/DX12/Layer.h>
#include <Backends/DX12/Ordinal.h>
#include <Backends/DX12/Shared.h>
#include <Backend/EnvironmentKeys.h>

// System
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <Psapi.h>
#include <intrin.h>

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

/// Use layer sessioning, useful for iteration
#ifndef NDEBUG
#   define USE_BOOTSTRAP_SESSIONS 1
#else // NDEBUG
#   define USE_BOOTSTRAP_SESSIONS 0
#endif // NDEBUG

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
SYMBOL(IHVAMDXC64ModuleName, "amdxc64.dll");

/// Function types
using PFN_GET_PROC_ADDRESS         = FARPROC (WINAPI*)(HMODULE hModule, LPCSTR lpProcName);
using PFN_LOAD_LIBRARY_A           = HMODULE (WINAPI*)(LPCSTR lpLibFileName);
using PFN_LOAD_LIBRARY_W           = HMODULE (WINAPI*)(LPCWSTR lpLibFileName);
using PFN_LOAD_LIBRARY_EX_A        = HMODULE (WINAPI*)(LPCSTR lpLibFileName, HANDLE, DWORD);
using PFN_LOAD_LIBRARY_EX_W        = HMODULE (WINAPI*)(LPCWSTR lpLibFileName, HANDLE, DWORD);
using PFN_CREATE_PROCESS_A         = BOOL (WINAPI*)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
using PFN_CREATE_PROCESS_W         = BOOL (WINAPI*)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
using PFN_CREATE_PROCESS_AS_USER_A = BOOL (WINAPI*)(HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
using PFN_CREATE_PROCESS_AS_USER_W = BOOL (WINAPI*)(HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

/// Next
PFN_GET_PROC_ADDRESS         Kernel32GetProcAddressOriginal;
PFN_LOAD_LIBRARY_A           Kernel32LoadLibraryAOriginal;
PFN_LOAD_LIBRARY_W           Kernel32LoadLibraryWOriginal;
PFN_LOAD_LIBRARY_EX_A        Kernel32LoadLibraryExAOriginal;
PFN_LOAD_LIBRARY_EX_W        Kernel32LoadLibraryExWOriginal;
PFN_CREATE_PROCESS_A         Kernel32CreateProcessAOriginal;
PFN_CREATE_PROCESS_W         Kernel32CreateProcessWOriginal;
PFN_CREATE_PROCESS_AS_USER_A Kernel32CreateProcessAsUserAOriginal;
PFN_CREATE_PROCESS_AS_USER_W Kernel32CreateProcessAsUserWOriginal;
D3D12GPUOpenFunctionTable    DetourFunctionTable;

/// Module lock-free acquisition states
uint32_t AMDAGSGuard{0u};
uint32_t D3D12Guard{0u};
uint32_t DXGIGuard{0u};
uint32_t D3D11Guard{0u};
uint32_t IHVAMDXC64Guard{0u};

/// Detoured section
struct DetourSection {
    /// Jump target address for the trampoline
    uint8_t* jmpBlockAddr{nullptr};

    /// Number of operands
    BYTE operandCount{0};
    
    /// Expected operands
    BYTE jmpOperands[5];
};

/// Detour sections
DetourSection Kernel32CreateProcessASection;
DetourSection Kernel32CreateProcessWSection;
DetourSection Kernel32CreateProcessAsUserASection;
DetourSection Kernel32CreateProcessAsUserWSection;

/// Bootstrapper path, relative to the current module
static std::string BootstrapperPathX64 = (GetCurrentModuleDirectory() / "GRS.Backends.DX12.BootstrapperX64.dll").string();
static std::string BootstrapperPathX32 = (GetCurrentModuleDirectory() / "GRS.Backends.DX12.BootstrapperX32.dll").string();

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

/// IHV modules
HMODULE IHVAMDXC64Module;

/// Special module ranges
MODULEINFO IHVAMDXC64ModuleInfo{};

/// Kernel32 module
HMODULE Kernel32Module{nullptr};

/// Layer function table
D3D12GPUOpenFunctionTable LayerFunctionTable;

/// Critical sections
CRITICAL_SECTION libraryCriticalSection{};
CRITICAL_SECTION bootstrapCriticalSection{};

/// Snapshot of a module set
using ModuleSnapshot = std::set<HMODULE>;

/// Well documented image base
extern "C" IMAGE_DOS_HEADER __ImageBase;

/// Prototypes
ModuleSnapshot GetModuleSnapshot();
bool DetourForeignModules(const ModuleSnapshot& before);
void DetourAMDAGSModule(HMODULE handle, bool insideTransaction);
void DetourD3D11Module(HMODULE handle, bool insideTransaction); 
void DetourD3D12Module(HMODULE handle, bool insideTransaction); 
void DetourDXGIModule(HMODULE handle, bool insideTransaction);
void CommitFunctionTable();

#if ENABLE_LOGGING
std::mutex LoggingLock;
std::wofstream LoggingFile;
#endif // ENABLE_LOGGING

/// Critical section guard helper
struct CriticalSectionGuard {
    CriticalSectionGuard(CRITICAL_SECTION& section) : section(section) {
        EnterCriticalSection(&section);
    }

    ~CriticalSectionGuard() {
        LeaveCriticalSection(&section);
    }

    CRITICAL_SECTION& section;
};

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
            LoggingFile.flush();
        }
        return *this;
    }
};
#endif // ENABLE_LOGGING

struct DetourJmpSectionTrapGuard {
    DetourJmpSectionTrapGuard(const DetourSection& section) : section(section) {
        /* Poof */
    }

    ~DetourJmpSectionTrapGuard() {
        // Modified jump block?
        if (!std::memcmp(section.jmpBlockAddr, &section.jmpOperands, section.operandCount)) {
            return;
        }

#if ENABLE_LOGGING
        LogContext{} << "DetourJmpSectionTrapGuard, jump block has changed\n";
#endif // ENABLE_LOGGING
        
        // Old page access
        DWORD pageAccessRestore{0};

        // Move page range to EXECUTE_READWRITE
        if (!VirtualProtect(section.jmpBlockAddr, section.operandCount, PAGE_EXECUTE_READWRITE, &pageAccessRestore)) {
            return;
        }

        // Restore the jump block
        std::memcpy(section.jmpBlockAddr, &section.jmpOperands, section.operandCount);

        // Restore the page protection flags
        DWORD ignore{0};
        VirtualProtect(section.jmpBlockAddr, section.operandCount, pageAccessRestore, &ignore);

        // Range flush the instruction cache
        FlushInstructionCache(GetCurrentProcess(), section.jmpBlockAddr, section.operandCount);
    }

    /// Current section
    const DetourSection& section;
};

DetourSection DetourAttachProtect(void** ppPointer, void* pDetour) {
    // Detouring location
    void* realTarget;

    // Perform detouring
    if (LONG result = DetourAttachEx(ppPointer, pDetour, nullptr, &realTarget, nullptr); result != NO_ERROR) {
        return {};
    }

    // Create section
    DetourSection section;
    section.jmpBlockAddr = static_cast<PBYTE>(realTarget);
    return section;
}

void CommitDetourSection(DetourSection& section) {
    // Set the operand count
#if defined(_WIN64) || defined(_WIN32)
    section.operandCount = 5;
#elif defined(_IA64_) || defined(_ARM_) // defined(_WIN64) || defined(_WIN32)
    section.operandCount = 8;
#elif defined(_ARM64_) // defined(_IA64_) || defined(_ARM_)
    section.operandCount = 12; 
#endif // defined(_ARM64_)
    
    // Copy section operands
    std::memcpy(&section.jmpOperands, section.jmpBlockAddr, sizeof(section.jmpOperands));
}

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
    // Ensure bootstrapping is serial
    CriticalSectionGuard guard(bootstrapCriticalSection);
    
    // No re-entry if an attempt has already been made
    if (HasInitializedOrFailed) {
        return;
    }

    // An attempt was made
    HasInitializedOrFailed = true;

    // Never bootstrap wow64 processes
    BOOL isWow64;
    if (IsWow64Process(GetCurrentProcess(), &isWow64) && isWow64) {
        // Just pretend that the function table is the bottom one
        LayerFunctionTable = DetourFunctionTable;
        
#if ENABLE_LOGGING
        LogContext{} << "Skipping bootstrapping, not supported for SysWow64\n";
#endif // ENABLE_LOGGING
        
        // Fire event
        if (!SetEvent(InitializationEvent))
        {
#if ENABLE_LOGGING
            LogContext{} << "Failed to release deferred initialization lock\n";
#endif // ENABLE_LOGGING
        }

        // Out!
        return;
    }

    // Get module path, the bootstrapper sessions are hosted under Intermediate
    std::filesystem::path modulePath = GetBaseModuleDirectory();

    // Add search directory
    AddDllDirectory(modulePath.wstring().c_str());

    // Process path
    std::filesystem::path path = modulePath / "GRS.Backends.DX12.Layer.dll";
#if ENABLE_LOGGING
    LogContext{} << invoker << " - Loading layer " << path << " ... ";
#endif // ENABLE_LOGGING

#if USE_BOOTSTRAP_SESSIONS
    // Get current session dir
    std::filesystem::path sessionDir = GetIntermediatePath("Bootstrapper\\Sessions");

    // Create unique name
    std::string sessionName = "GRS.Backends.DX12.Layer " + GlobalUID::New().ToString() + ".dll";

    // Copy the bootstrapper to a new session, makes handling unique sessions somewhat bearable (certain programs refuse to let go of handle)
    std::filesystem::path sessionPath = sessionDir / sessionName;

    // Copy current bootstrapper
    std::filesystem::copy(path, sessionPath);
#else // USE_BOOTSTRAP_SESSIONS
    // No copy
    std::filesystem::path sessionPath = path;
#endif // USE_BOOTSTRAP_SESSIONS

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();
    
    // User attempting to load instrumentable object, warranting bootstrapping of the layer
    LayerModule = Kernel32LoadLibraryExWOriginal(sessionPath.wstring().c_str(), nullptr, 0x0);

    // Query embedded hooks
    DetourForeignModules(snapshot);
    
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

void WINAPI D3D12GetGPUOpenBootstrapperInfo(D3D12GPUOpenBootstrapperInfo* out) {
    out->version = 1;
}

void OnDetourModule(HMODULE& module, HMODULE handle) {
    ASSERT(module == nullptr, "Re-entrant detouring");
    module = handle;
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

bool IsServiceActive() {
    // Try to open the service handle
    HANDLE handle = OpenMutex(MUTEX_ALL_ACCESS, false, kSharedD3D12ServiceMutexName);

    // If the handle exists the service is active
    if (handle) {
        CloseHandle(handle);
        return true;
    }

    // Bootstrapper may still be valid if there's an environment token, implies that it was launched from the toolkit
    size_t size;
    if (char* tokenKey{nullptr}; _dupenv_s(&tokenKey, &size, Backend::kReservedEnvironmentTokenKey) == 0 && tokenKey) {
        free(tokenKey);
        return true;
    }

    // Not active
    return false;
}

void ServiceTrap() {
    // Running?
    if (IsServiceActive()) {
        return;
    }
    
    // Fire event just in case some module has locked ours
    if (!SetEvent(InitializationEvent))
    {
#if ENABLE_LOGGING
        LogContext{} << "Failed to release deferred initialization lock\n";
#endif // ENABLE_LOGGING
    }
    
#if ENABLE_LOGGING
    LogContext{} << "\tService trap triggered!\n";
#endif // ENABLE_LOGGING

    // Unload the bootstrapper
    FreeLibraryAndExitThread(reinterpret_cast<HINSTANCE>(&__ImageBase), 0);
}

bool GetBootstrapperForArch(HANDLE process, const char** out) {
    // Determine if process is wow64
    BOOL isWow64;
    if (!IsWow64Process(process, &isWow64)) {
        return false;
    }
    
    // Set bootstrapper
    if (isWow64) {
        *out = BootstrapperPathX32.data();
    } else {
        *out = BootstrapperPathX64.data();
    }

    // OK
    return true;
}

BOOL BootstrapSuspendedProcessA(PROCESS_INFORMATION* lpProcessInformation, DWORD dwCreationFlags, bool owner) {
    const char* dlls[] = { nullptr };
    
#if ENABLE_LOGGING
    LogContext{} << "\tBootstrapSuspendedProcessA\n";
#endif // ENABLE_LOGGING

    // Determine bootstrapper
    if (!GetBootstrapperForArch(lpProcessInformation->hProcess, &dlls[0])) {
#if ENABLE_LOGGING
        LogContext{} << "\tArch Indeterminate\n";
#endif // ENABLE_LOGGING
        return false;
    }
    
#if ENABLE_LOGGING
    LogContext{} << "\t" << dlls[0] << "\n";
#endif // ENABLE_LOGGING

    // Try to bootstrap
    if (!DetourUpdateProcessWithDll(lpProcessInformation->hProcess, dlls, 1u) &&
        !DetourProcessViaHelperDllsA(lpProcessInformation->dwProcessId, 1u, dlls, Kernel32CreateProcessAOriginal)) {
        // Critical failure! Kill the process
        TerminateProcess(lpProcessInformation->hProcess, ~0u);
    
        // Release handles
        CloseHandle(lpProcessInformation->hProcess);
        CloseHandle(lpProcessInformation->hThread);

        // Failed
        return false;
    }

    // Resume if not requested suspended
    if (!(dwCreationFlags & CREATE_SUSPENDED)) {
        ResumeThread(lpProcessInformation->hThread);
    }

    // If the owner, release the handle
    if (owner) {
        CloseHandle(lpProcessInformation->hProcess);
        CloseHandle(lpProcessInformation->hThread);
    }

    // OK
    return true;
}

BOOL BootstrapSuspendedProcessW(PROCESS_INFORMATION* lpProcessInformation, DWORD dwCreationFlags, bool owner) {
    const char* dlls[] = { nullptr };
    
#if ENABLE_LOGGING
    LogContext{} << "\tBootstrapSuspendedProcessW\n";
#endif // ENABLE_LOGGING

    // Determine bootstrapper
    if (!GetBootstrapperForArch(lpProcessInformation->hProcess, &dlls[0])) {
#if ENABLE_LOGGING
        LogContext{} << "\tArch Indeterminate\n";
#endif // ENABLE_LOGGING
        return false;
    }
    
#if ENABLE_LOGGING
    LogContext{} << "\t" << dlls[0] << "\n";
#endif // ENABLE_LOGGING

    // Try to bootstrap
    if (!DetourUpdateProcessWithDll(lpProcessInformation->hProcess, dlls, 1u) &&
        !DetourProcessViaHelperDllsW(lpProcessInformation->dwProcessId, 1u, dlls, Kernel32CreateProcessWOriginal)) {
#if ENABLE_LOGGING
        LogContext{} << "\tInjection failed, terminating!\n";
#endif // ENABLE_LOGGING
        
        // Critical failure! Kill the process
        TerminateProcess(lpProcessInformation->hProcess, ~0u);

        // Release handles
        CloseHandle(lpProcessInformation->hProcess);
        CloseHandle(lpProcessInformation->hThread);

        // Failed
        return false;
    }

    // Resume if not requested suspended
    if (!(dwCreationFlags & CREATE_SUSPENDED)) {
#if ENABLE_LOGGING
        LogContext{} << "\tResuming\n";
#endif // ENABLE_LOGGING
        
        ResumeThread(lpProcessInformation->hThread);
    }

    // If the owner, release the handle
    if (owner) {
#if ENABLE_LOGGING
        LogContext{} << "\tReleasing\n";
#endif // ENABLE_LOGGING
        
        CloseHandle(lpProcessInformation->hProcess);
        CloseHandle(lpProcessInformation->hThread);
    }

    // OK
    return true;
}

BOOL WINAPI HookCreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    PROCESS_INFORMATION processInfo{};

    // Detour jump trap guard
    DetourJmpSectionTrapGuard guard(Kernel32CreateProcessASection);
    
#if ENABLE_LOGGING
    LogContext{} << "HookCreateProcessA-Detour\n\t'" << (lpApplicationName ? lpApplicationName : "<no-app>") << "'\n\t'" << (lpCommandLine ? lpCommandLine : "<no-app>") << "'\n";
#endif // ENABLE_LOGGING

    // Should we bootstrap the process?
    bool bootstrapProcess = IsServiceActive();

    // Creation flags
    DWORD creationFlags = dwCreationFlags;
    if (bootstrapProcess) {
        creationFlags |= CREATE_SUSPENDED;
    }

    // Use local process info if none requested
    if (lpProcessInformation == nullptr) {
        lpProcessInformation = &processInfo;
    }
    
    // Start process suspended
    if (!Kernel32CreateProcessAOriginal(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        creationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    )) {
#if ENABLE_LOGGING
        LogContext{} << "\tBottom Failed\n";
#endif // ENABLE_LOGGING
        return false;
    }

    // Boostrap it
    if (bootstrapProcess) {
        return BootstrapSuspendedProcessA(lpProcessInformation, dwCreationFlags, lpProcessInformation == &processInfo);
    }

    // OK
    return true;
}

BOOL WINAPI HookCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    PROCESS_INFORMATION processInfo{};

    // Detour jump trap guard
    DetourJmpSectionTrapGuard guard(Kernel32CreateProcessWSection);
    
#if ENABLE_LOGGING
    LogContext{} << "HookCreateProcessW-Detour\n\t'"
                 << "Application Name : "<< (lpApplicationName ? lpApplicationName : L"<no-app>") << "'\n\t'"
                 << "Command Line     : "<< (lpCommandLine ? lpCommandLine : L"<no-app>") << "'\n\t"
                 << "Creation         : " << dwCreationFlags << "\n\t"
                 << "Inherit Handles  : " << bInheritHandles << "\n";
#endif // ENABLE_LOGGING

    // Should we bootstrap the process?
    bool bootstrapProcess = IsServiceActive();

    // Creation flags
    DWORD creationFlags = dwCreationFlags;
    if (bootstrapProcess) {
        creationFlags |= CREATE_SUSPENDED;
    }

    // Use local process info if none requested
    if (lpProcessInformation == nullptr) {
        lpProcessInformation = &processInfo;
    }

    // Start process suspended
    if (!Kernel32CreateProcessWOriginal(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        creationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    )) {
#if ENABLE_LOGGING
        LogContext{} << "\tBottom Failed\n";
#endif // ENABLE_LOGGING
        return false;
    }

    // Boostrap it
    if (bootstrapProcess) {
        return BootstrapSuspendedProcessW(lpProcessInformation, dwCreationFlags, lpProcessInformation == &processInfo);
    }

    // OK
    return true;
}

BOOL WINAPI HookCreateProcessAsUserA(HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    PROCESS_INFORMATION processInfo{};

    // Detour jump trap guard
    DetourJmpSectionTrapGuard guard(Kernel32CreateProcessAsUserASection);
    
#if ENABLE_LOGGING
    LogContext{} << "HookCreateProcessAsUserA-Detour\n\t'" << (lpApplicationName ? lpApplicationName : "<no-app>") << "'\n\t'" << (lpCommandLine ? lpCommandLine : "<no-app>") << "'\n";
#endif // ENABLE_LOGGING

    // Should we bootstrap the process?
    bool bootstrapProcess = IsServiceActive();

    // Creation flags
    DWORD creationFlags = dwCreationFlags;
    if (bootstrapProcess) {
        creationFlags |= CREATE_SUSPENDED;
    }

    // Use local process info if none requested
    if (lpProcessInformation == nullptr) {
        lpProcessInformation = &processInfo;
    }
    
    // Start process suspended
    if (!Kernel32CreateProcessAsUserAOriginal(
        hToken,
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        creationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    )) {
#if ENABLE_LOGGING
        LogContext{} << "\tBottom Failed\n";
#endif // ENABLE_LOGGING
        return false;
    }

    // Boostrap it
    if (bootstrapProcess) {
        return BootstrapSuspendedProcessA(lpProcessInformation, dwCreationFlags, lpProcessInformation == &processInfo);
    }

    // OK
    return true;
}

BOOL WINAPI HookCreateProcessAsUserW(HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    PROCESS_INFORMATION processInfo{};

    // Detour jump trap guard
    DetourJmpSectionTrapGuard guard(Kernel32CreateProcessAsUserWSection);
    
#if ENABLE_LOGGING
    LogContext{} << "HookCreateProcessAsUserW-Detour\n\t'" << (lpApplicationName ? lpApplicationName : L"<no-app>") << "'\n\t'" << (lpCommandLine ? lpCommandLine : L"<no-app>") << "'\n";
#endif // ENABLE_LOGGING

    // Should we bootstrap the process?
    bool bootstrapProcess = IsServiceActive();

    // Creation flags
    DWORD creationFlags = dwCreationFlags;
    if (bootstrapProcess) {
        creationFlags |= CREATE_SUSPENDED;
    }
    
    // Use local process info if none requested
    if (lpProcessInformation == nullptr) {
        lpProcessInformation = &processInfo;
    }
    
    // Start process suspended
    if (!Kernel32CreateProcessAsUserWOriginal(
        hToken,
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        creationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
    )) {
#if ENABLE_LOGGING
        LogContext{} << "\tBottom Failed\n";
#endif // ENABLE_LOGGING
        return false;
    }

    // Boostrap it
    if (bootstrapProcess) {
        return BootstrapSuspendedProcessW(lpProcessInformation, dwCreationFlags, lpProcessInformation == &processInfo);
    }

    // OK
    return true;
}

void QueryIHVAMDXC64(HMODULE handle) {
    // Keep a reference to it
    OnDetourModule(IHVAMDXC64Module, handle);

    // Get module info
    if (!GetModuleInformation(GetCurrentProcess(), handle, &IHVAMDXC64ModuleInfo, sizeof(IHVAMDXC64ModuleInfo))) {
        IHVAMDXC64ModuleInfo = {};
    }
}

bool TryLoadEmbeddedModules(HMODULE handle) {
    bool any = false;
    
#if ENABLE_LOGGING
    LogContext{} << "\tTryLoadEmbeddedModules!\n";
#endif // ENABLE_LOGGING
    
    // Get the base name
    char baseName[1024];
    if (!GetModuleBaseNameA(GetCurrentProcess(), handle, baseName, sizeof(baseName))) {
        return false;
    }

#if ENABLE_LOGGING
    LogContext{} << "TryLoadEmbeddedModules, " << baseName << "\n";
#endif // ENABLE_LOGGING
    
    // Is AGS?
    if (Kernel32GetProcAddressOriginal(handle, "agsDriverExtensionsDX12_CreateDevice") != nullptr && InterlockedCompareExchange(&AMDAGSGuard, 1u, 0u) == 0u) {
        DetourAMDAGSModule(handle, false);
        any = true;
    }

    // Is D3D12?
    if (!std::strcmp(baseName, kD3D12ModuleName) && Kernel32GetProcAddressOriginal(handle, "D3D12CreateDevice") != nullptr && InterlockedCompareExchange(&D3D12Guard, 1u, 0u) == 0u) {
        DetourD3D12Module(handle, false);
        any = true;
    }

    // Is DXGI?
    if (!std::strcmp(baseName, kDXGIModuleName) && Kernel32GetProcAddressOriginal(handle, "CreateDXGIFactory") != nullptr && InterlockedCompareExchange(&DXGIGuard, 1u, 0u) == 0u) {
        DetourDXGIModule(handle, false);
        any = true;
    }

    // Is D3D11?
    if (!std::strcmp(baseName, kD3D11ModuleName) && Kernel32GetProcAddressOriginal(handle, "D3D11On12CreateDevice") != nullptr && InterlockedCompareExchange(&D3D11Guard, 1u, 0u) == 0u) {
        DetourD3D11Module(handle, false);
        any = true;
    }

    // Is AMD Driver?
    if (!std::strcmp(baseName, kIHVAMDXC64ModuleName) && InterlockedCompareExchange(&IHVAMDXC64Guard, 1u, 0u) == 0u) {
        QueryIHVAMDXC64(handle);
    }

    // OK
    return any;
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
    for (size_t i = 0; i < std::min<size_t>(needed / sizeof(HMODULE), modules.size()); i++) {
        snapshot.insert(modules[i]);
    }

    // OK
    return snapshot;
}

bool DetourForeignModules(const ModuleSnapshot& before) {
    bool any = false;

#ifdef THIN_X86
    return false;
#endif
    
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
        any |= TryLoadEmbeddedModules(module);
    }

    // OK
    return any;
}

HMODULE WINAPI HookLoadLibraryA(LPCSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();
    
    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryAOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    if (DetourForeignModules(snapshot)) {
        BootstrapLayer("HookLoadLibraryA");
    }

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryWOriginal(lpLibFileName);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    if (DetourForeignModules(snapshot)) {
        BootstrapLayer("HookLoadLibraryW");
    }

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExA(LPCSTR lpLibFileName, HANDLE handle, DWORD flags) {
#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExA '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExAOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    if (DetourForeignModules(snapshot)) {
        BootstrapLayer("HookLoadLibraryExA");
    }

    // OK
    return module;
}

HMODULE WINAPI HookLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE handle, DWORD flags) {
    // Initial snapshot
    ModuleSnapshot snapshot = GetModuleSnapshot();

#if ENABLE_LOGGING
    LogContext{} << "HookLoadLibraryExW '" << lpLibFileName << "'\n";
#endif // ENABLE_LOGGING

    // Pass down call chain, preserve error stack
    HMODULE module = Kernel32LoadLibraryExWOriginal(lpLibFileName, handle, flags);
    if (!module) {
        return module;
    }

    // Query embedded hooks
    if (DetourForeignModules(snapshot)) {
        BootstrapLayer("HookLoadLibraryExW");
    }

    // OK
    return module;
}

DWORD WINAPI DeferredInitialization(void*) {
    // Check service
    ServiceTrap();

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

    // Header
    LogContext{} << "PID " << GetProcessId(GetCurrentProcess()) << "\n";

    // Function table
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
        // ! Call native LoadLibraryW, not detoured
        BootstrapLayer("Entry detected mounted d3d12 module");

        // Query embedded hooks
        DetourForeignModules({});
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

bool IsModuleRegion(const MODULEINFO& info, const void* address) {
    auto addressValue = reinterpret_cast<size_t>(address);
    auto moduleValue  = reinterpret_cast<size_t>(info.lpBaseOfDll);

    // Check if in bounds
    return
        info.lpBaseOfDll &&
        (addressValue >= moduleValue) &&
        (addressValue < moduleValue + info.SizeOfImage);
}

bool IsIHVRegion(const void* address) {
    return IsModuleRegion(IHVAMDXC64ModuleInfo, address);
}

HRESULT WINAPI HookID3D12CreateDevice(
    _In_opt_ IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void **ppDevice) {
    // Keep parent frame
    const void* callee = _ReturnAddress();

    // Hold for deferred initialization, must happen before IHV region checks due to foreign modules
    WaitForDeferredInitialization();

    // If this is not an IHV region, just pass down the call-chain
    if (!IsIHVRegion(callee)) {
        return LayerFunctionTable.next_D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, riid, ppDevice);
    }

    // Native device
    ID3D12Device* device{nullptr};

    // Create device with special vendor IID, this device is not wrapped
    if (HRESULT hr = LayerFunctionTable.next_D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, kIIDD3D12DeviceVendor, reinterpret_cast<void**>(&device)); FAILED(hr)) {
        return hr;
    }

    // Unwrap to expected IID
    return device->QueryInterface(riid, ppDevice);
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

void DetourAMDAGSModule(HMODULE handle, bool insideTransaction) {
    OnDetourModule(AMDAGSModule, handle);
    
#if ENABLE_LOGGING
    LogContext{} << "\tDetourAMDAGSModule!\n";
#endif // ENABLE_LOGGING
    
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
    
#if ENABLE_LOGGING
    LogContext{} << "\tDetourD3D12Module!\n";
#endif // ENABLE_LOGGING
    
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
    
#if ENABLE_LOGGING
    LogContext{} << "\tDetourD3D11Module!\n";
#endif // ENABLE_LOGGING
    
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
    
#if ENABLE_LOGGING
    LogContext{} << "\tDetourDXGIModule!\n";
#endif // ENABLE_LOGGING
    
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
[[maybe_unused]]
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
        if (dwReason == DLL_PROCESS_ATTACH) {
            DetourRestoreAfterWith();
        }

        // OK
        return TRUE;
    }

#if ENABLE_WHITELIST
    bool isWhitelisted = false;

    // Check if any of the target applications
    for (const char* name : kWhitelist) {
        if (std::icontains(GetCurrentExecutableName(), name)) {
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

#if 0
        // Ensure the bootstrapper stays pinned in the process, re-entrant bootstrapping is a mess
        PinBootstrapper();
#endif // 0

        // Create critical section
        InitializeCriticalSection(&libraryCriticalSection);
        InitializeCriticalSection(&bootstrapCriticalSection);

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
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, kKernel32ModuleNameW, &Kernel32Module)) {
            // Nothing we are interested in
            return false;
        }

        // Get addresses
        Kernel32LoadLibraryAOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_A>(GetProcAddress(Kernel32Module, "LoadLibraryA"));
        Kernel32LoadLibraryWOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_W>(GetProcAddress(Kernel32Module, "LoadLibraryW"));
        Kernel32LoadLibraryExAOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_EX_A>(GetProcAddress(Kernel32Module, "LoadLibraryExA"));
        Kernel32LoadLibraryExWOriginal = reinterpret_cast<PFN_LOAD_LIBRARY_EX_W>(GetProcAddress(Kernel32Module, "LoadLibraryExW"));
        Kernel32GetProcAddressOriginal = reinterpret_cast<PFN_GET_PROC_ADDRESS>(GetProcAddress(Kernel32Module, "GetProcAddress"));
        
#ifndef THIN_X86
        // Attach against original address
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryAOriginal), reinterpret_cast<void*>(HookLoadLibraryA));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryWOriginal), reinterpret_cast<void*>(HookLoadLibraryW));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExAOriginal), reinterpret_cast<void*>(HookLoadLibraryExA));
        DetourAttach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExWOriginal), reinterpret_cast<void*>(HookLoadLibraryExW));
        DetourAttach(&reinterpret_cast<void *&>(Kernel32GetProcAddressOriginal), reinterpret_cast<void *>(HookGetProcAddress));
#endif // THIN_X86

        // Attach against original address
        Kernel32CreateProcessAOriginal = reinterpret_cast<PFN_CREATE_PROCESS_A>(GetProcAddress(Kernel32Module, "CreateProcessA"));
        Kernel32CreateProcessASection = DetourAttachProtect(&reinterpret_cast<void *&>(Kernel32CreateProcessAOriginal), reinterpret_cast<void *>(HookCreateProcessA));

        // Attach against original address
        Kernel32CreateProcessWOriginal = reinterpret_cast<PFN_CREATE_PROCESS_W>(GetProcAddress(Kernel32Module, "CreateProcessW"));
        Kernel32CreateProcessWSection = DetourAttachProtect(&reinterpret_cast<void *&>(Kernel32CreateProcessWOriginal), reinterpret_cast<void *>(HookCreateProcessW));

        // Attach against original address
        Kernel32CreateProcessAsUserAOriginal = reinterpret_cast<PFN_CREATE_PROCESS_AS_USER_A>(GetProcAddress(Kernel32Module, "CreateProcessAsUserA"));
        Kernel32CreateProcessAsUserASection = DetourAttachProtect(&reinterpret_cast<void *&>(Kernel32CreateProcessAsUserAOriginal), reinterpret_cast<void *>(HookCreateProcessAsUserA));

        // Attach against original address
        Kernel32CreateProcessAsUserWOriginal = reinterpret_cast<PFN_CREATE_PROCESS_AS_USER_W>(GetProcAddress(Kernel32Module, "CreateProcessAsUserW"));
        Kernel32CreateProcessAsUserWSection = DetourAttachProtect(&reinterpret_cast<void *&>(Kernel32CreateProcessAsUserWOriginal), reinterpret_cast<void *>(HookCreateProcessAsUserW));

        // Attempt to create initial detours
        DetourForeignModules(ModuleSnapshot {});

        // Commit all transactions
        if (FAILED(DetourTransactionCommit())) {
            return FALSE;
        }

        // Commit all sections
        CommitDetourSection(Kernel32CreateProcessASection);
        CommitDetourSection(Kernel32CreateProcessWSection);
        CommitDetourSection(Kernel32CreateProcessAsUserASection);
        CommitDetourSection(Kernel32CreateProcessAsUserWSection);
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
#ifndef THIN_X86
            DetourDetach(&reinterpret_cast<void*&>(Kernel32GetProcAddressOriginal), reinterpret_cast<void*>(HookGetProcAddress));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryAOriginal), reinterpret_cast<void*>(HookLoadLibraryA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryWOriginal), reinterpret_cast<void*>(HookLoadLibraryW));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExAOriginal), reinterpret_cast<void*>(HookLoadLibraryExA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32LoadLibraryExWOriginal), reinterpret_cast<void*>(HookLoadLibraryExW));
#endif // THIN_X86
            DetourDetach(&reinterpret_cast<void*&>(Kernel32CreateProcessAOriginal), reinterpret_cast<void*>(HookCreateProcessA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32CreateProcessWOriginal), reinterpret_cast<void*>(HookCreateProcessW));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32CreateProcessAsUserAOriginal), reinterpret_cast<void*>(HookCreateProcessAsUserA));
            DetourDetach(&reinterpret_cast<void*&>(Kernel32CreateProcessAsUserWOriginal), reinterpret_cast<void*>(HookCreateProcessAsUserW));

            // Detach initial creation
#ifndef THIN_X86
            DetachInitialCreation();
#endif // THIN_X86

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

        // Release critical section
        DeleteCriticalSection(&libraryCriticalSection);
        DeleteCriticalSection(&bootstrapCriticalSection);
    }

    // OK
    return TRUE;
}

#ifndef THIN_X86
extern "C" __declspec(dllexport) LRESULT WinHookAttach(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
#endif
