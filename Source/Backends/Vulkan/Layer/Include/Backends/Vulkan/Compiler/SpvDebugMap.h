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
#include <Backends/Vulkan/Compiler/Spv.h>
#include <Backends/Vulkan/Compiler/SpvSourceAssociation.h>

// Common
#include <Common/Assert.h>

// Std
#include <vector>
#include <string_view>
#include <span>

struct SpvDebugVariableInfo {
    /// Id of this variable
    SpvId varId = IL::InvalidID;
    
    /// Name of this variable
    SpvId nameId = IL::InvalidID;
    
    /// Type of this variable
    SpvId typeId = IL::InvalidID;
};
    
struct InstructionValueInfo {
    /// Debug variable were storing to
    SpvId debugVariableId = IL::InvalidID;
    
    /// Value being stored
    SpvId value = IL::InvalidID;
    
    /// Originating expression
    SpvId expression = IL::InvalidID;
    
    /// Optional structural indices
    std::span<const SpvId> accessIndices{};
};
    
struct SpvDebugInstructionValueSetInfo {
    /// All values associated with this instruction
    std::vector<InstructionValueInfo> values;
};
    
struct SpvDebugBindingInfo {
    /// Debug variable bound to this instance
    SpvId debugVariable = IL::InvalidID;
};

struct SpvDebugTypeInfo {
    /// Kind of this type
    SpvOp kind = {};
    
    /// Optional operands
    const SpvId* operands = nullptr;
    
    /// Number of operands
    uint32_t opCount = 0;
};

struct SpvDebugMap {
    /// Set the id bound
    /// \param id
    void SetBound(SpvId id) {
        entries.resize(id);
    }

    /// Add a new debug entry
    /// \param id spv identifier
    /// \param op original opcode
    /// \param str assigned string
    void Add(SpvId id, SpvOp op, const std::string_view& str) {
        Entry& entry = entries.at(id);
        ASSERT(entry.op == SpvOpNop, "Double debug additional");

        entry.op = op;
        entry.value = str;
    }

    /// Get a debug string and verify the original opcode
    /// \param id spv identifier
    /// \param op insertion opcode
    /// \return the debug string
    const std::string_view& Get(SpvId id, SpvOp op) const {
        const Entry& entry = entries.at(id);
        ASSERT(entry.op == op, "Unexpected op code");
        return entry.value;
    }

    /// Get a debug string
    /// \param id spv identifier
    /// \return the debug string
    const std::string_view& GetValue(SpvId id) const {
        return entries.at(id).value;
    }

    /// Get the opcode for a given debug identifier
    /// \param id spv identifier
    /// \return opcode, Nop if not inserted
    SpvOp GetOpCode(SpvId id) const {
        return entries.at(id).op;
    }

    /// All debug variables
    std::unordered_map<SpvId, SpvDebugVariableInfo> variableInfos;
    
    /// All instruction associations
    std::unordered_map<uint32_t, SpvDebugInstructionValueSetInfo> instructionValueInfos;

    /// All variable bindings
    std::unordered_map<SpvId, SpvDebugBindingInfo> bindingInfos;
    
    /// All debug types
    std::unordered_map<SpvId, SpvDebugTypeInfo> typeInfos;

private:
    struct Entry {
        SpvOp op{SpvOpNop};
        std::string_view value;
    };

    /// All entries
    std::vector<Entry> entries;
};
