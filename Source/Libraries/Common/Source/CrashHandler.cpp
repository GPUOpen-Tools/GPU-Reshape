#include <Common/CrashHandler.h>
#include <Common/Sink.h>

// Std
#include <mutex>

// Win32
#ifdef _WIN32
#   ifndef NDEBUG
#       define WIN32_EXCEPTION_HANDLER
#       include <iostream>
#       include <Windows.h>
#       include <dbghelp.h>
#   endif
#endif

#ifdef WIN32_EXCEPTION_HANDLER
static LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    static std::mutex globalLock;
    std::lock_guard guard(globalLock);

    // Create console
    AllocConsole();

    FILE* stream{};
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    // Notify user
    std::cerr << "Crash detected, current frames:\n";

    // Common handles
    HANDLE thread  = GetCurrentThread();
    HANDLE process = GetCurrentProcess();

    // Ensure symbols are ready
    SymInitialize(process, NULL, TRUE);

    // Module handles
    HMODULE currentModule{nullptr};
    HMODULE lastModule{nullptr};

    // Current displacement
    DWORD64 displacement{0};

    // Current frame
    STACKFRAME64 stack;
    memset(&stack, 0, sizeof(STACKFRAME64));

    // Walk frame
    for (ULONG frame = 0;; frame++) {
        GRS_SINK(frame);

        BOOL result = StackWalk64(
#if defined(_M_AMD64)
            IMAGE_FILE_MACHINE_AMD64,
#else
            IMAGE_FILE_MACHINE_I386,
#endif
            process,
            thread,
            &stack,
            pExceptionInfo->ContextRecord,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        // Failed?
        if (!result) {
            break;
        }

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];

        // Symbol info
        PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        SymFromAddr(process, static_cast<ULONG64>(stack.AddrPC.Offset), &displacement, pSymbol);

        // Line info
        IMAGEHLP_LINE64 *line = static_cast<IMAGEHLP_LINE64 *>(malloc(sizeof(IMAGEHLP_LINE64)));
        line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        // Print symbol & address
        std::cerr << "\t[" << reinterpret_cast<const void*>(pSymbol->Address) << "] " << pSymbol->Name;

        // Next module?
        if (!currentModule || lastModule != currentModule) {
            lastModule = currentModule;
            currentModule = nullptr;

            // Module path
            char module[256];
            lstrcpyA(module, "");

            // Try to get the module
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(stack.AddrPC.Offset), &currentModule);

            // Success?
            if (currentModule != nullptr) {
                GetModuleFileNameA(currentModule, module, 256);
            }

            std::cerr << "\n\t\t" << module;
        }

        // Get line, if possible
        DWORD disp;
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line)) {
            std::cerr << "\n\t\t" << line->FileName << " line " << line->LineNumber;
        }

        // Next!
        std::cerr << "\n";
        free(line);
    }

    std::cerr << "\nWaiting for debugger to attach... " << std::flush;

    // Wait for the debugger
    while (!IsDebuggerPresent()) {
        Sleep(100);
    }

    // Notify user
    std::cerr << "Attached." << std::endl;

    // Break!
    DebugBreak();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void SetDebugCrashHandler() {
    // Platform handler
#ifdef WIN32_EXCEPTION_HANDLER
    if (!IsDebuggerPresent()) {
        SetUnhandledExceptionFilter(TopLevelExceptionHandler);
    }
#endif
}
