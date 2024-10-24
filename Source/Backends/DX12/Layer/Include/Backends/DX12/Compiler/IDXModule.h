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
#include <Backends/DX12/Compiler/DXCodeOffsetTraceback.h>

// Common
#include <Common/GlobalUID.h>

// Std
#include <cstdint>

// Forward declarations
struct DXParseJob;
struct DXCompileJob;
struct DXStream;
struct IDxcLibrary;
class IDXDebugModule;
class IDXCompilerEnvironment;
namespace IL {
    struct Program;
}

/// Base DXModule
class IDXModule {
public:
    virtual ~IDXModule() = default;

    /// Scan the DXIL bytecode
    /// \param job job
    /// \return success state
    virtual bool Parse(const DXParseJob& job) = 0;

    /// Copy this module
    /// \return
    virtual IDXModule* Copy() = 0;

    /// Recompile this module
    /// \param out lifetime owned by this module
    /// \return
    virtual bool Compile(const DXCompileJob& job, DXStream& out) = 0;

    /// Get the program of this module
    /// \return program
    virtual IL::Program* GetProgram() = 0;

    /// Get the debug information
    /// \return debug interface
    virtual IDXDebugModule* GetDebug() = 0;

    /// Check if the debug module is "slim"
    /// \return true if slim
    virtual bool IsSlimDebugModule() = 0;

    /// Create a compiler environment
    /// \param library target library
    /// \return new library, must be free'd
    virtual IDXCompilerEnvironment* CreateCompilerEnvironment(IDxcLibrary* library) = 0;

    /// Get a source traceback
    /// \param codeOffset given offset, must originate from the same module
    /// \return traceback
    virtual DXCodeOffsetTraceback GetCodeOffsetTraceback(uint32_t codeOffset) = 0;
    
    /// Get the guid
    /// \return
    virtual GlobalUID GetInstrumentationGUID() = 0;

    /// Get the language
    /// \return given language
    virtual const char* GetLanguage() = 0;
};
