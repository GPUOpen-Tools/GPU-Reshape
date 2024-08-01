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

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Entry point physical block
struct SpvPhysicalBlockEntryPoint : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

    /// Compile all new instructions
    void Compile();

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination capability
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockEntryPoint& out);

public:
    /// Add a shadewr interface value
    /// \param id identifier to be added to entry point interfaces
    void AddInterface(SpvId id) {
        for (EntryPoint& entryPoint : entryPoints) {
            entryPoint.interfaces.push_back(id);
        }
    }
    
    /// Add a shader interface value
    /// \param storageClass variable storage class
    /// \param id identifier to be added to entry point interfaces
    void AddInterface(SpvStorageClass storageClass, SpvId id);

private:
    struct ExecutionMode {
        /// Execution modes may use different opcodes
        SpvOp opCode;
        
        /// Annotated execution mode
        SpvExecutionMode executionMode;

        /// Given payload for this execution mode
        TrivialStackVector<uint32_t, 8> payload;
    };
    
    struct EntryPoint {
        /// Identifier of this entrypoint
        SpvId id;
        
        /// Assigned execution model
        SpvExecutionModel executionModel;

        /// Entrypoint name
        std::string name;

        /// All interfaces
        std::vector<SpvId> interfaces;

        /// All execution modes
        std::vector<ExecutionMode> executionModes;
    };

    /// Compile an entry point
    /// \param entryPoint entry point to compile
    void CompileEntryPoint(const EntryPoint& entryPoint);

    /// Compile an execution mode, literal/id derived from opcode
    /// \param entryPoint target entry point
    /// \param executionMode the mode to compile
    void CompileExecutionMode(SpvId entryPoint, ExecutionMode &executionMode);

    /// Get an entry point
    /// \param id id to search for 
    /// \return nullptr if not found
    EntryPoint* GetEntryPoint(SpvId id);

private:
    /// All entrypoints
    std::vector<EntryPoint> entryPoints;
};
