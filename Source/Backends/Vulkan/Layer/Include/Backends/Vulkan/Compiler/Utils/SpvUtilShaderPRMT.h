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
    /// \param function the owning function (used for source traceback)
    /// \param resource source resource
    /// \param result output result
    void GetToken(const SpvJob& job, SpvStream& stream, IL::ID function, IL::ID resource, IL::ID result);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader export
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderPRMT& out);

public:
    /// Create a PRM control structure for a resource
    /// \param job source job
    /// \param stream the current spirv stream
    /// \param function the owning function (used for source traceback)
    /// \param resource source resource
    /// \return PRM control structure
    IL::ID CreatePRMControl(const SpvJob& job, SpvStream& stream, IL::ID function, IL::ID resource);

    /// Get the PRM control type
    const Backend::IL::Type *GetPRMControlType() const {
        return prmControl;
    }

private:
    struct SpvPRMTOffset {
        /// Computed offsets
        IL::ID metadataOffset{IL::InvalidID};
        IL::ID descriptorOffset{IL::InvalidID};

        /// Parent table was not bound
        IL::ID tableNotBound{IL::InvalidID};

        /// Optional bounds operand
        IL::ID outOfBounds{IL::InvalidID};
    };

    /// Get the PRMT offset for a given resource
    /// \param job source job
    /// \param stream current stream
    /// \param function the owning function (used for source traceback)
    /// \param resource resource to be tracked
    /// \return allocated identifier
    SpvPRMTOffset GetResourcePRMTOffset(const SpvJob& job, SpvStream& stream, IL::ID function, IL::ID resource);

private:
    struct DynamicSpvValueDecoration {
        /// Descriptor set index
        /// Kept in IL ID's as it may be dynamic or not
        IL::ID descriptorSet{IL::InvalidID};

        /// Descriptor wise offset, used for PRM offsets
        IL::ID descriptorOffset{IL::InvalidID};

        /// Dynamic offset into source
        IL::ID dynamicOffset{IL::InvalidID};

        /// Does the dynamic offset require bounds checking?
        bool checkOutOfBounds{false};
    };

    /// Find the originating resource decoration
    /// \param job source job
    /// \param function the owning function (used for source traceback)
    /// \param resource resource to be traced
    /// \return found decoration
    DynamicSpvValueDecoration GetSourceResourceDecoration(const SpvJob& job, SpvStream& stream, IL::ID function, IL::ID resource);

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
    const Backend::IL::Type *prmControl{nullptr};
};
