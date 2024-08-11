// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Backends/DX12/Compiler/DXMSCompiler.h>
#include <Backends/DX12/Layer.h>
#include <Backends/DX12/Compiler/DXCompilerArgument.h>
#include <Backends/DX12/Compiler/IDXCompilerEnvironment.h>
#include <Backends/DX12/Compiler/IDXModule.h>

// System
#include <Windows.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Containers/TrivialStackVector.h>
#include <Common/String.h>

// Std
#include <string>

// Special includes
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

bool DXMSCompiler::Install() {
    // Get path of the layer
    std::filesystem::path modulePath = GetBaseModuleDirectory() / "Dependencies" / "DXC";

    // Load dxil
    //   ! No non-system/runtime dependents in dxil.dll, verified with dumpbin
    dxilModule = LoadLibrary((modulePath / "GRS.dxil.dll").string().c_str());
    if (!dxilModule) {
        return false;
    }

    // Load dxcompiler
    //   ! No non-system/runtime dependents in dxcompiler.dll, verified with dumpbin
    dxCompilerModule = LoadLibrary((modulePath / "GRS.dxcompiler.dll").string().c_str());
    if (!dxCompilerModule) {
        return false;
    }

    // Get gpa's
    auto gpaDXILCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxilModule, "DxcCreateInstance"));
    auto gpaDXCCreateInstance = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(dxCompilerModule, "DxcCreateInstance"));

    // Missing?
    if (!gpaDXILCreateInstance || !gpaDXCCreateInstance) {
        return false;
    }

    // Try to create a library instance
    if (FAILED(gpaDXCCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&library))) {
        return false;
    }
    
    // Try to create a compiler
    if (FAILED(gpaDXCCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), (void **)&compiler))) {
        return false;
    }

    // OK
    return true;
}

IDxcResult* DXMSCompiler::CompileWithEmbeddedDebug(IDXModule* module) {
    // Create the environment
    IDXCompilerEnvironment *environment = module->CreateCompilerEnvironment(library.Get());
    
    // Enumerate all compiler arguments
    std::vector<std::wstring> arguments;
    EnumerateArguments(environment, arguments);

    // Unwrap all arguments
    TrivialStackVector<LPCWSTR, 32> argumentPtrs;
    for (const std::wstring& argument : arguments) {
        argumentPtrs.Add(argument.c_str());
    }

    // Get source code
    std::string_view source = environment->GetSourceContents();

    // Try to create an in-place blob for the source code
    DxcBuffer sourceBuffer{};
    sourceBuffer.Encoding = DXC_CP_ACP;
    sourceBuffer.Ptr = source.data();
    sourceBuffer.Size = source.length();

    // Destination blob 
    IDxcResult *shaderBlob = nullptr;

    // Try to compile the contents
    HRESULT hr = compiler->Compile(
        &sourceBuffer,
        argumentPtrs.Data(),
        static_cast<uint32_t>(argumentPtrs.Size()),
        environment->GetDxcIncludeHandler(),
        __uuidof(IDxcResult),
        reinterpret_cast<void**>(&shaderBlob)
    );

    // Cleanup
    destroy(environment, allocators);

    // Failed?
    if (FAILED(hr)) {
        return nullptr;
    }

    // OK
    return shaderBlob;
}

void DXMSCompiler::EnumerateArguments(IDXCompilerEnvironment *environment, std::vector<std::wstring> &out) {
    // Get number of arguments
    uint32_t argumentCount;
    environment->EnumerateArguments(&argumentCount, nullptr);

    // Setup container
    TrivialStackVector<DXCompilerArgument, 32> arguments;
    arguments.Resize(argumentCount);

    // Get all arguments
    environment->EnumerateArguments(&argumentCount, arguments.Data());

    // Append all name, value pairs
    for (const DXCompilerArgument& argument : arguments) {
        std::string key(argument.name);

        // Ignored source arguments
        if (std::iequals(key, "Zs") ||
            std::iequals(key, "Zi") ||
            std::iequals(key, "Fd") ||
            std::iequals(key, "Qstrip_debug") ||
            std::iequals(key, "Qstrip_reflect")) {
            continue;
        }

        // Compose argument name
        std::wstringstream wstr;
        wstr << L"/";
        wstr << std::wstring(argument.name.data(), argument.name.data() + argument.name.length());
        out.emplace_back(wstr.str());

        // Compose argument value
        if (!argument.value.empty()) {
            out.emplace_back(argument.value.data(), argument.value.data() + argument.value.length());
        }
    }

    // Embed all debug data
    out.push_back(L"/Zi");
    out.push_back(L"/Qembed_debug");
    out.push_back(L"/Qsource_in_debug_module");
}

DXMSCompiler::~DXMSCompiler() {
    compiler.Reset();
    library.Reset();

    if (dxilModule) {
        FreeLibrary(dxilModule);
    }

    if (dxCompilerModule) {
        FreeLibrary(dxCompilerModule);
    }
}
