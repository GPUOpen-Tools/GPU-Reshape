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
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Program.h>

// Std
#include <set>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct SpvPhysicalBlockScan;
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;

/// Debug string source basic block
struct SpvPhysicalBlockDebugStringSource : public SpvPhysicalBlockSection {
    SpvPhysicalBlockDebugStringSource(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable &table);

    /// Parse all instructions
    void Parse();

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination debug string source
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockDebugStringSource& out);

    /// Get the linear file index
    /// \param id string identifier
    uint32_t GetFileIndex(SpvId id) {
        return sourceMap.GetFileIndex(id, debugMap.Get(id, SpvOpString));
    }
    
    /// SPIRV debug map
    SpvDebugMap debugMap;

    /// SPIRV source map
    SpvSourceMap sourceMap;
};
