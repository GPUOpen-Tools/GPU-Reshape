// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <Backends/Vulkan/Compiler/Blocks/SpvValueDecoration.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockScan.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/Program.h>
#include <Backend/IL/ID.h>

// Forward declarations
struct SpvJob;
struct SpvPhysicalBlockTable;

/// Shader PRMT utilities
struct SpvUtilShaderPRMT {
    SpvUtilShaderPRMT(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Compile the records
    /// \param job source job being compiled against
    void CompileRecords(const SpvJob &job);

    /// Export a given value
    /// \param job source job
    /// \param stream the current spirv stream
    /// \param resource source resource
    /// \param result output result
    void GetToken(const SpvJob& job, SpvStream& stream, IL::ID resource, IL::ID result);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader export
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderPRMT& out);

private:
    struct SpvPRMTOffset {
        /// Computed offset
        IL::ID offset{IL::InvalidID};

        /// Optional bounds operand
        IL::ID outOfBounds{IL::InvalidID};
    };

    /// Get the PRMT offset for a given resource
    /// \param job source job
    /// \param stream current stream
    /// \param resource resource to be tracked
    /// \return allocated identifier
    SpvPRMTOffset GetResourcePRMTOffset(const SpvJob& job, SpvStream& stream, IL::ID resource);

private:
    struct DynamicSpvValueDecoration {
        /// Source operand
        SpvValueDecoration source;

        /// Dynamic offset into source
        IL::ID dynamicOffset{IL::InvalidID};

        /// Does the dynamic offset require bounds checking?
        bool checkOutOfBounds{false};
    };

    /// Find the originating resource decoration
    /// \param job source job
    /// \param resource resource to be traced
    /// \return found decoration
    DynamicSpvValueDecoration GetSourceResourceDecoration(const SpvJob& job, SpvStream& stream, IL::ID resource);

private:
    /// Shared allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Parent table
    SpvPhysicalBlockTable& table;

    /// Spv identifiers
    uint32_t prmTableId{0};

    /// Type map
    const Backend::IL::Type *buffer32UIPtr{nullptr};
    const Backend::IL::Type *buffer32UI{nullptr};
};
