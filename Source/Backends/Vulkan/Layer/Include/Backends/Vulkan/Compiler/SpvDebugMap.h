// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Common
#include <Common/Assert.h>

// Std
#include <vector>
#include <string_view>

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

private:
    struct Entry {
        SpvOp op{SpvOpNop};
        std::string_view value;
    };

    /// All entries
    std::vector<Entry> entries;
};
