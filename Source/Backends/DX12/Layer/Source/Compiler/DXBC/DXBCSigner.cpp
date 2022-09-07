#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>

// Common
#include <Common/FileSystem.h>

// System
#include <Windows.h>

// Std
#include <string>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

bool DXBCSigner::Install() {
    // Get path of the layer
    std::filesystem::path modulePath = GetCurrentModuleDirectory();

    // Load DXBC
    //   ! No non-system/runtime dependents in dxbcsigner.dll, verified with dumpbin
    module = LoadLibrary((modulePath / "dxbcsigner.dll").string().c_str());
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
