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
#include <Backends/Vulkan/Compiler/SpvConstantMap.h>
#include <Backends/Vulkan/Compiler/SpvTypeMap.h>
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/SpvBlock.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Program.h>

// Std
#include <set>

// Forward declarations
struct SpvPhysicalBlockTable;
struct SpvParseContext;
struct SpvPhysicalBlock;
struct SpvRecordReader;
struct SpvJob;

/// Type constant and variable physical block
struct SpvPhysicalBlockTypeConstantVariable : public SpvPhysicalBlockSection {
    SpvPhysicalBlockTypeConstantVariable(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Parse all instructions
    void Parse();

    /// Assign all type associations for a given instruction, ensures types are mapped correctly
    /// \param ctx parsing context
    void AssignTypeAssociation(const SpvParseContext& ctx);

    /// Assign all type associations for a given instruction, ensures types are mapped correctly
    /// \param ctx parsing context
    void AssignTypeAssociation(const SpvRecordReader& ctx);

    /// Compile the block
    /// \param idMap the shared identifier map for proxies
    void Compile(SpvIdMap& idMap);

    /// Compile all pending constants
    void CompileConstants();

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination type constant variable
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockTypeConstantVariable& out);

    /// SPIRV type map
    SpvTypeMap typeMap;

    /// SPIRV constant map
    SpvConstantMap constantMap;

public:
    /// Create the PC block
    /// \param job source job
    /// \return identifier
    IL::ID CreatePushConstantBlock(const SpvJob& job);

    /// Get the member offset
    uint32_t GetPushConstantMemberOffset() const {
        return pushConstantMemberOffset;
    }

    /// Get the block type
    const Backend::IL::Type* GetPushConstantBlockType() const {
        return pushConstantBlockType;
    }

    /// Get the variable id
    uint32_t GetPushConstantVariableId() const {
        return pushConstantVariableId;
    }

private:
    SpvBlock recordBlock;

private:
    /// PC structure type
    const Backend::IL::Type* pushConstantBlockType{nullptr};

    /// PC user member offset
    uint32_t pushConstantMemberOffset = 0;

    /// PC variable
    uint32_t pushConstantVariableId = IL::InvalidOffset;

    /// PC variable offset for patching
    uint32_t pushConstantVariableOffset = IL::InvalidOffset;
};
