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
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/SpvCodeOffsetTraceback.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Program.h>
#include <Backend/IL/TypeCommon.h>

// Std
#include <set>

// Forward declarations
struct SpvPhysicalBlockScan;
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;
struct SpvParseContext;
struct SpvIdMap;
struct SpvJob;
struct SpvStream;

/// Function definition and declaration physical block
struct SpvPhysicalBlockFunction : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

    /// Parse the function header
    /// \param function destination function
    /// \param ctx parsing context
    void ParseFunctionHeader(IL::Function *function, SpvParseContext &ctx);

    /// Parse the function body
    /// \param function destination function
    /// \param ctx parsing context
    void ParseFunctionBody(IL::Function *function, SpvParseContext &ctx);

    /// Compile the physical block
    /// \param job source job
    /// \param idMap
    /// \return success state
    bool Compile(const SpvJob& job, SpvIdMap& idMap);

    /// Compile a function
    /// \param job source job
    /// \param idMap the shared identifier map for proxies
    /// \param fn the function to be compiled
    /// \param emitDefinition if true, the definition body will get emitted
    /// \return success state
    bool CompileFunction(const SpvJob& job, SpvIdMap& idMap, IL::Function& fn, bool emitDefinition);

    /// Compile a basic block
    /// \param job source job
    /// \param idMap the shared identifier map for proxies
    /// \param bb the basic block to be recompiled
    /// \return success state
    bool CompileBasicBlock(const SpvJob& job, SpvIdMap& idMap, IL::Function& fn, IL::BasicBlock* bb, bool isModifiedScope);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination function
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockFunction& out);

    /// Get the code offset traceback
    /// \param codeOffset code offset, must originate from same function
    /// \return traceback
    SpvCodeOffsetTraceback GetCodeOffsetTraceback(uint32_t codeOffset);

private:
    /// Patch all loop continues
    /// \param fn function
    void PostPatchLoopContinue(IL::Function* fn);

    /// Post patch a loop user instruction
    /// \param instruction user instruction
    /// \param original original block
    /// \param redirect the block to be redirected to
    /// \return false if not applicable
    bool PostPatchLoopContinueInstruction(IL::Instruction* instruction, IL::ID original, IL::ID redirect);

    /// Patch the selection merge construct instruction
    /// \param instruction the instruction
    /// \param original original merge block to test for
    /// \param redirect the block to redirect to
    /// \return true if redirected
    bool PostPatchLoopSelectionMergeInstruction(IL::Instruction* instruction, IL::ID original, IL::ID redirect);

    /// Patch all selection construct merges
    /// \param outerUser the outer user to test for selection merges
    /// \param bridgeBlockId the block to patch against
    void PostPatchLoopSelectionMerge(const IL::OpaqueInstructionRef& outerUser, IL::ID bridgeBlockId);

    /// Create the user fed data map
    /// \param job parent job
    void CreateDataResourceMap(const SpvJob& job);

    /// Create the user fed data map
    /// \param job parent job
    void CreateDataConstantMap(const SpvJob& job, SpvStream& stream, SpvIdMap& idMap);

    /// Create the data lookups
    /// \param job parent job
    /// \param stream function scope
    /// \param idMap identifier map to be used for the current scope
    void CreateDataLookups(const SpvJob& job, SpvStream& stream, SpvIdMap& idMap);

private:
    /// Check if a given instruction is trivially copyable, including special instructions which
    /// may require recompilation.
    /// \param bb source basic block
    /// \param it instruction iterator
    /// \return true if trivially copyable
    bool IsTriviallyCopyableSpecial(IL::BasicBlock *bb, const IL::BasicBlock::Iterator& it);

    /// Migrate a combined image sampler state
    /// \param stream source stream
    /// \param idMap compilation identifier map
    /// \param bb source basic block
    /// \param instr sampling instruction
    /// \return image identifier
    IL::ID MigrateCombinedImageSampler(SpvStream &stream, SpvIdMap &idMap, IL::BasicBlock *bb, const IL::SampleTextureInstruction *instr);
    
private:
    /// Identifier type
    enum class IdentifierType {
        None,
        CombinedImageSampler,
        SampleTexture
    };

    /// Single identifier metadata
    struct IdentifierMetadata {
        /// Given type
        IdentifierType type = IdentifierType::None;

        /// Payload
        union {
            struct {
                IL::ID type;
                IL::ID image;
                IL::ID sampler;
            } combinedImageSampler;

            struct {
                IL::ID combinedType;
                IL::ID combinedImageSampler;
            } sampleImage;
        };
    };

    /// All metadata
    std::vector<IdentifierMetadata> identifierMetadata;

private:
    /// All traceback information
    std::unordered_map<uint32_t, SpvCodeOffsetTraceback> sourceTraceback;

private:
    struct LoopContinueBlock {
        IL::InstructionRef<> instruction;
        IL::ID block;
    };

    /// All continue blocks
    std::vector<LoopContinueBlock> loopContinueBlocks;
};
