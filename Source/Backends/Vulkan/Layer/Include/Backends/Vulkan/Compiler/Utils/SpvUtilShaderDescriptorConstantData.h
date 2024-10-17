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

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/Program.h>

// Forward declarations
struct SpvJob;
struct SpvPhysicalBlockTable;

/// Shader PRMT utilities
struct SpvUtilShaderDescriptorConstantData {
    SpvUtilShaderDescriptorConstantData(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Compile the records
    /// \param job source job being compiled against
    void CompileRecords(const SpvJob &job);

    /// Export a given value
    /// \param stream the current spirv stream
    /// \param offset the element wise offset
    /// \param index the dword index
    IL::ID GetDescriptorData(SpvStream& stream, IL::ID offset, uint32_t index);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader block
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderDescriptorConstantData& out);

    /// Set the push constant identifier
    /// \param id given id
    void SetPCID(IL::ID id) {
        pcId = id;
    }

private:
    /// Shared allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Parent table
    SpvPhysicalBlockTable& table;

    /// Spv identifiers
    uint32_t descriptorConstantId{0};

    /// Push constant offset identifier
    IL::ID pcId{IL::InvalidID};
};
