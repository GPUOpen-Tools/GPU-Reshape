#pragma once

// Layer
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>

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
    /// \param idMap
    /// \return success state
    bool Compile(SpvIdMap& idMap);

    /// Compile a function
    /// \param idMap the shared identifier map for proxies
    /// \param fn the function to be compiled
    /// \param emitDefinition if true, the definition body will get emitted
    /// \return success state
    bool CompileFunction(SpvIdMap& idMap, IL::Function& fn, bool emitDefinition);

    /// Compile a basic block
    /// \param idMap the shared identifier map for proxies
    /// \param bb the basic block to be recompiled
    /// \return success state
    bool CompileBasicBlock(SpvIdMap& idMap, IL::BasicBlock* bb);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination function
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockFunction& out);

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

private:
    struct LoopContinueBlock {
        IL::InstructionRef<IL::BranchConditionalInstruction> branchConditional;
        IL::ID block;
    };

    /// All continue blocks
    std::vector<LoopContinueBlock> loopContinueBlocks;
};
