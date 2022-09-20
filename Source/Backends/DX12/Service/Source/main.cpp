#include <Backends/DX12/Shared.h>

// Common
#include <Common/IPGlobalLock.h>
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <cstdint>
#include <iostream>

/// Shared Win32 hook
HHOOK Hook;

/// Graceful exit handler
BOOL CtrlHandler(DWORD event) {
    if (event == CTRL_CLOSE_EVENT) {
        // Graceful cleanup
        if (Hook) {
            UnhookWindowsHookEx(Hook);
        }

        // OK
        return TRUE;
    }

    // Ignore
    return FALSE;
}

/// Naive pump for hooks
void MessagePump() {
    // Pooled message
    MSG message;

    // Keep the message pump active for all hooked applications
    for (;;) {
        int32_t pumpResult = GetMessage(&message, NULL, 0, 0);
        switch (pumpResult) {
            default:
                TranslateMessage(&message);
                DispatchMessage(&message);

                // Erroneous
            case 0:
            case -1:
                return;
        }
    }
}

int main(int32_t argc, const char *const *argv) {
    std::cout << "GPUOpen DX12 Service\n" << std::endl;
    std::cout << "Initializing global lock... " << std::flush;

    // Set handlers
    SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(CtrlHandler), true);

    // Try to acquire lock
    IPGlobalLock globalLock;
    if (!globalLock.Acquire(kSharedD3D12ServiceMutexName, true)) {
        std::cerr << "Failed to open or create shared mutex '" << kSharedD3D12ServiceMutexName << "'" << std::endl;

#ifndef NDEBUG
        std::cin.ignore();
#endif
        return 1;
    }

    std::cout << "OK." << std::endl;

    // Host all sessions under intermediate
    std::filesystem::path sessionDir = GetIntermediatePath("Bootstrapper/Sessions");

    // Clean up all old sessions
    for (std::filesystem::path file: std::filesystem::directory_iterator(sessionDir)) {
        std::error_code ignored;
        std::filesystem::remove(file, ignored);
    }

    // Create unique name
    std::string sessionName = "Backends.DX12.Bootstrapper " + GlobalUID::New().ToString() + ".dll";

    // Copy the bootstrapper to a new session, makes handling unique sessions somewhat bearable (certain programs refuse to let go of handle)
    std::filesystem::path sessionPath = sessionDir / sessionName;

    // Copy current bootstrapper
    std::filesystem::copy("Backends.DX12.Bootstrapper.dll", sessionPath);

    // Load the boostrapper
    HMODULE bootstrapperModule = LoadLibraryW(sessionPath.wstring().c_str());
    if (!bootstrapperModule) {
        std::cerr << "Failed to open bootstrapper" << std::endl;

#ifndef NDEBUG
        std::cin.ignore();
#endif
        return 1;
    }

    // Get hook proc
    auto gpa = reinterpret_cast<HOOKPROC>(GetProcAddress(bootstrapperModule, "WinHookAttach"));
    if (!gpa) {
        std::cerr << "Failed to get bootstrapper WinHookAttach address" << std::endl;

#ifndef NDEBUG
        std::cin.ignore();
#endif
        return 1;
    }

    // Attempt to attach global hook
    Hook = SetWindowsHookEx(WH_CBT, gpa, bootstrapperModule, 0);
    if (!Hook) {
        DWORD error = GetLastError();

        // Format the underlying error
        char buffer[1024];
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buffer, sizeof(buffer), NULL
        );

        std::cerr << "Failed to attach global hook: " << buffer << std::endl;

#ifndef NDEBUG
        std::cin.ignore();
#endif
        return 1;
    }

    // Hold and start pump
    std::cout << "Holding hook..." << std::endl;
    MessagePump();

    // Cleanup
    UnhookWindowsHookEx(Hook);

    // OK
    std::cout << "DX12 service shutdown" << std::endl;
    return 0;
}
