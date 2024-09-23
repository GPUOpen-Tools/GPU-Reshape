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
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockScan.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

// Backend
#include <Backend/IL/Program.h>

// Forward declarations
struct SpvJob;
struct SpvParseContext;
struct SpvRecordReader;
struct SpvPhysicalBlockTable;

/// Shader debug utilities
struct SpvUtilShaderDebug {
    SpvUtilShaderDebug(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable& table);

    /// Parse the module
    void Parse();

    /// Parse an instruction
    /// \param ctx parse context
    void ParseInstruction(SpvParseContext& ctx);

    /// Finalize all sources
    /// Should be done before function parsing
    void FinalizeSource();

public:
    /// Parse a module debug100 instruction
    /// \param ctx record context
    void ParseDebug100Instruction(SpvRecordReader& ctx);

    /// Parse a debug100 function instruction
    /// \param ctx parsing context
    /// \param sourceAssociation current source association, may be changed
    void ParseDebug100FunctionInstruction(SpvParseContext& ctx, SpvSourceAssociation& sourceAssociation);

    /// Check if an instruction set is debug100
    bool IsDebug100(IL::ID set) const {
        return set == extDebugInfo100;
    }
    
public:
    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader block
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderDebug& out);

    /// Get the linear file index
    /// \param id string identifier
    uint32_t GetFileIndex(SpvId id) {
        return sourceMap.GetFileIndex(id, debugMap.Get(id, SpvOpString));
    }

    /// SPIRV debug map
    SpvDebugMap debugMap;

    /// SPIRV source map
    SpvSourceMap sourceMap;

private:
    /// Shared allocators
    Allocators allocators;

    /// Backend program
    IL::Program &program;

    /// Parent table
    SpvPhysicalBlockTable& table;

private:
    /// Queried debug 100 info
    IL::ID extDebugInfo100{IL::InvalidID};

    /// Compilation unit information
    struct Debug100CompilationUnit {
        uint32_t          version;
        uint32_t          dwarfVersion;
        SpvId             source100Id;
        SpvSourceLanguage language;
    } debug100CompilationUnit{};

    /// Associated metadata for debug 100 instructions
    union Debug100Metadata {
        struct {
            uint32_t fileIndex;
        } debugSource;
    };

    /// Get the metadata for a debug100 instruction
    Debug100Metadata& GetDebug100Metadata(SpvId id);

    /// All debug100 metadata
    std::vector<Debug100Metadata> debug100Metadata;

private:
    // Current source being processed
    uint32_t pendingSource{};
};
