// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include "SpvInstruction.h"
#include "SpvPhysicalBlock.h"
#include "SpvHeader.h"

// Generated
#include <Backends/Vulkan/Compiler/Spv.Gen.h>

// Backend
#include <Backend/IL/Program.h>

// Std
#include <algorithm>

/// Physical block scanner
struct SpvPhysicalBlockScan {
    SpvPhysicalBlockScan(IL::Program& program);

    /// Scan a spirv stream
    /// \param code stream begin
    /// \param count stream word count
    /// \return success state
    bool Scan(const uint32_t *const code, uint32_t count);

    /// Get the expected module word count
    /// \return word count
    uint64_t GetModuleWordCount() const;

    /// Stitch a program
    /// \param out the destination stream
    void Stitch(SpvStream& out);

    /// Copy this block scanner
    /// \param out destination scanner
    void CopyTo(SpvPhysicalBlockScan& out);

    /// Get a physical block
    /// \param type given type of the block
    /// \return the physical block
    SpvPhysicalBlock* GetPhysicalBlock(SpvPhysicalBlockType type) {
        Section& section = sections[static_cast<uint32_t>(type)];
        return &section.physicalBlock;
    }

    /// SPIRV header
    SpvHeader header;

private:
    struct Section {
        SpvPhysicalBlock physicalBlock;
    };

    /// Backend program
    IL::Program& program;

    /// All sections
    Section sections[static_cast<uint32_t>(SpvPhysicalBlockType::Count)];
};
