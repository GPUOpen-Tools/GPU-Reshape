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

#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Common
#include <Common/IComponent.h>

// DXC
#include <DXC/dxcapi.h>

// System
#include <wrl/client.h>

// Std
#include <vector>

// Forward declarations
class IDXModule;
class IDXCompilerEnvironment;

class DXMSCompiler : public TComponent<DXMSCompiler> {
public:
    COMPONENT(DXCCompiler);

    /// Deconstructor
    ~DXMSCompiler();

    /// Install this compiler
    /// \return false if failed
    bool Install();

    /// Compile a new module with embedded debug data
    /// \param module source module
    /// \return nullptr if failed
    IDxcResult* CompileWithEmbeddedDebug(IDXModule* module);

private:
    /// Enumerate all module source arguments
    /// \param environment source environment
    /// \param out destination arguments
    void EnumerateArguments(IDXCompilerEnvironment* environment, std::vector<std::wstring>& out);

private:
    /// Objects
    Microsoft::WRL::ComPtr<IDxcLibrary> library;
    Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;

private:
    /// Dynamic modules
    HMODULE dxilModule{nullptr};
    HMODULE dxCompilerModule{nullptr};
};
