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

/// Capability physical block
struct SpvPhysicalBlockCapability : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

    /// Add a new capability
    /// \param capability capability to be added
    void Add(SpvCapability capability);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination capability
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockCapability& out);

private:
    /// Current set of capabilities
    std::set<SpvCapability> capabilities;
};
