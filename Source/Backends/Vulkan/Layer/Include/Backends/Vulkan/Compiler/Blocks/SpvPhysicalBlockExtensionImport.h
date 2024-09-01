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
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/Spv.h>

// Backend
#include <Backend/IL/Program.h>

// Common
#include <Common/ShortHashString.h>

// Forward declarations
struct SpvPhysicalBlockScan;
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;

/// Extension import physical block
struct SpvPhysicalBlockExtensionImport : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

    /// Get an existing instruction set or invalid
    /// \param name target name
    /// \return instruction set id
    IL::ID Get(ShortHashString name);

    /// Get or add a new instruction set
    /// \param name target name
    /// \return instruction set id
    IL::ID GetOrAdd(ShortHashString name);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination capability
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockExtensionImport& out);

private:
    struct InstructionSet {
        /// Given name
        ShortHashString name;

        /// Assigned id
        IL::ID id{IL::InvalidID};
    };

    /// All instruction sets
    std::vector<InstructionSet> sets;
};
