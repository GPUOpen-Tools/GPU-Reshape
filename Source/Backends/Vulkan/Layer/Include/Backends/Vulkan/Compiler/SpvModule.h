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

// Common
#include <Common/Assert.h>
#include <Common/GlobalUID.h>

// Std
#include <set>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/Emitters/Emitter.h>

// Layer
#include <Backends/Vulkan/Compiler/SpvTypeMap.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>
#include <Backends/Vulkan/Config.h>
#include <Backends/Vulkan/Compiler/SpvCodeOffsetTraceback.h>

// Forward declarations
struct SpvDebugMap;
struct SpvSourceMap;
struct SpvPhysicalBlockTable;

class SpvModule {
public:
    SpvModule(const Allocators& allocators, uint64_t shaderGUID, const GlobalUID& instrumentationGUID = GlobalUID::New());
    ~SpvModule();

    /// No copy
    SpvModule(const SpvModule& other) = delete;
    SpvModule& operator=(const SpvModule& other) = delete;

    /// No move
    SpvModule(SpvModule&& other) = delete;
    SpvModule& operator=(SpvModule&& other) = delete;

    /// Copy this module
    /// \return
    SpvModule* Copy() const;

    /// Parse a module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    bool ParseModule(const uint32_t* code, uint32_t wordCount);

    /// Specialize a module
    /// \param job target job
    /// \return success state
    bool Specialize(const SpvJob& job);

    /// Recompile the program, code must be the same as the originally parsed module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    /// \return success state
    bool Recompile(const uint32_t* code, uint32_t wordCount, const SpvJob& job);

    /// Get code offset traceback
    /// \param codeOffset code offset, must originate from this module
    /// \return traceback
    SpvCodeOffsetTraceback GetCodeOffsetTraceback(uint32_t codeOffset);

    /// Get the produced program
    /// \return
    const IL::Program* GetProgram() const {
        return program;
    }

    /// Get the produced program
    /// \return
    IL::Program* GetProgram() {
        return program;
    }

    /// Get the code pointer
    const uint32_t* GetCode() const {
        return spirvProgram.GetData();
    }

    /// Get the byte size of the code
    uint64_t GetSize() const {
        return spirvProgram.GetWordCount() * sizeof(uint32_t);
    }

    /// Get the source map for this module
    const SpvSourceMap* GetSourceMap() const;

    /// Get the parent module
    const SpvModule* GetParent() const {
        return parent;
    }

    const GlobalUID& GetInstrumentationGUID() const {
        return instrumentationGUID;
    }

private:
    /// Parent instance
    const SpvModule* parent{nullptr};

private:
    Allocators allocators;

    /// Global GUID
    uint64_t shaderGUID{~0ull};

    /// Instrumentation GUID
    GlobalUID instrumentationGUID;

    /// Debugging GUID name
#if SHADER_COMPILER_DEBUG
    std::string instrumentationGUIDName;
#endif

    /// JIT'ed program
    SpvStream spirvProgram;

    /// The physical block table, contains all spirv data
    SpvPhysicalBlockTable* physicalBlockTable{nullptr};

    /// Abstracted program
    IL::Program* program{nullptr};
};
