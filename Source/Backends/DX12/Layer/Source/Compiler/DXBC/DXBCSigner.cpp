#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>

// Std
#include <string>

// System
#include <Windows.h>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

bool DXBCSigner::Install() {
    // Load DXBC
    module = LoadLibraryExW(L"dxbcsigner.dll", nullptr, LOAD_LIBRARY_SEARCH_USER_DIRS);
    if (!module) {
        return false;
    }

    // Get gpa
    dxbcSign = reinterpret_cast<PFN_DXBCSign>(GetProcAddress(module, "SignDxbc"));

    // OK
    return true;
}

DXBCSigner::~DXBCSigner() {
    if (module) {
        FreeLibrary(module);
    }
}

bool DXBCSigner::Sign(void *code, uint64_t length) {
    return SUCCEEDED(dxbcSign(static_cast<BYTE*>(code), length));
}
