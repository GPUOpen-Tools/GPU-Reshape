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

#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/BasicBlockCommon.h>
#include <Backend/IL/ControlFlow.h>
#include <Backend/IL/InstructionCommon.h>

// Common
#include <Common/Alloca.h>
#include <Common/Containers/TrivialStackVector.h>

void IL::BasicBlock::CopyTo(BasicBlock *out) const {
    out->count = count;
    out->dirty = dirty;
    out->data = data;
    out->sourceSpan = sourceSpan;
    out->flags = flags;

    // Preallocate
    out->relocationTable.resize(relocationTable.size());

    // Copy the relocation offsets
    for (size_t i = 0; i < relocationTable.size(); i++) {
        out->relocationTable[i] = out->relocationAllocator.Allocate();
        out->relocationTable[i]->offset = relocationTable[i]->offset;
    }
}

void IL::BasicBlock::IndexUsers() {
    // Reindex the map
    for (auto it = begin(); it != end(); it++) {
        AddInstructionReferences(it.Get(), it);
    }
}

void IL::BasicBlock::AddInstructionReferences(const IL::Instruction *instruction, const IL::OpaqueInstructionRef &ref) {
    // Add reference to result
    if (instruction->result != InvalidID) {
        map.AddInstruction(ref, instruction->result);
    }

    // Add reference to blocks
    switch (instruction->opCode) {
        default:
            break;

        case OpCode::Phi: {
            auto* phi = instruction->As<PhiInstruction>();

            // Add use case
            for (uint32_t i = 0; i < phi->values.count; i++) {
                const IL::PhiValue& value = phi->values[i];
                map.AddBlockUser(value.branch, ref);
            }
            break;
        }

        case OpCode::Branch: {
            auto* branch = instruction->As<BranchInstruction>();
            map.AddBlockUser(branch->branch, ref);
            break;
        }

        case OpCode::BranchConditional: {
            auto* branch = instruction->As<BranchConditionalInstruction>();
            map.AddBlockUser(branch->pass, ref);
            map.AddBlockUser(branch->fail, ref);

            if (branch->controlFlow.merge != InvalidID) {
                map.AddBlockUser(branch->controlFlow.merge, ref);

                if (branch->controlFlow._continue != InvalidID) {
                    map.AddBlockUser(branch->controlFlow._continue, ref);
                }
            }

            break;
        }
    }
}

static bool IsAnyPhiBranchBackEdge(const IL::PhiInstruction* instr, const IL::BranchControlFlow& controlFlow) {
    if (controlFlow._continue == IL::InvalidID) {
        return false;
    }

    // Check if any branch matches
    for (uint32_t i = 0; i < instr->values.count; i++) {
        if (instr->values[i].branch == controlFlow._continue) {
            return true;
        }
    }

    // None found
    return false;
}

IL::BasicBlock::Iterator IL::BasicBlock::Split(IL::BasicBlock *destBlock, const IL::BasicBlock::Iterator &splitIterator, BasicBlockSplitFlagSet splitFlags) {
    ASSERT(destBlock->IsEmpty(), "Cannot split into a filled basic block");

    // The actual split iterator
    Iterator splitIteratorPhi = splitIterator;

    // Redirect backedges?
    if (splitFlags & BasicBlockSplitFlag::RedirectLoopBackedge) {
        // Are we splitting the terminator too?
        if (splitIteratorPhi != end()) {
            Iterator terminator = GetTerminator();

            // Is this a loop header we're splitting?
            BranchControlFlow controlFlow;
            if (Backend::IL::GetControlFlow(terminator.Get(), controlFlow) && controlFlow._continue != InvalidID) {
                BasicBlock* continueBlock = map.GetBasicBlock(controlFlow._continue);

                // Get the terminator in the continue block
                auto* continueBranch = continueBlock->GetTerminator().GetMutable()->Cast<BranchInstruction>();
                ASSERT(continueBranch, "Continue blocks must contain a single branch");

                // Remap the back-edge to the new loop header
                continueBranch->branch = destBlock->GetID();
            }
        }
    }

    bool hasUnresolvedBackEdgePhis = false;

    // Splitting phis?
    if (splitIteratorPhi->Is<PhiInstruction>() && splitFlags & BasicBlockSplitFlag::SplitPhiEdges) {
        BranchControlFlow controlFlow;
        Backend::IL::GetControlFlow(GetTerminator(), controlFlow);

        // Does the phi operation reference a backedge?
        bool hasBackEdge = IsAnyPhiBranchBackEdge(splitIterator->As<PhiInstruction>(), controlFlow);
        hasUnresolvedBackEdgePhis = hasBackEdge;

        // Validate that all phi operations have the same status
        // We're making quite a few assumptions on this, so it must hold true
#ifndef NDEBUG
        for (auto it = splitIteratorPhi; it != destBlock->end() && it->Is<PhiInstruction>(); ++it) {
            ASSERT(IsAnyPhiBranchBackEdge(it->As<PhiInstruction>(), controlFlow) == hasBackEdge, "Mismatch in back-edge status");
        }
#endif // !NDEBUG

        // If there's no back edges, preserve the phi operations, do not split with them
        if (!hasBackEdge) {
            splitIteratorPhi = Backend::IL::FirstNonPhi(this, splitIteratorPhi);
        }
    }

    // Byte offset to the split point
    uint32_t splitPointRelocationOffset = relocationTable[splitIteratorPhi.relocationIndex]->offset;

    // Redirect all branch users if requested
    if (splitFlags & BasicBlockSplitFlag::RedirectBranchUsers) {
        TrivialStackVector<IL::OpaqueInstructionRef, 128> removed(allocators);

        for (const OpaqueInstructionRef& ref : map.GetBlockUsers(GetID())) {
            IL::Instruction* instruction = ref.basicBlock->GetRelocationInstruction(ref.relocationOffset);

            // Handle user instruction
            switch (instruction->opCode) {
                default: {
                    // Skip this one, not of interest
                    continue;
                }
                case OpCode::BranchConditional: {
                    auto* branch = instruction->As<BranchConditionalInstruction>();

                    // We may be splitting a continue block
                    if (branch->controlFlow._continue != GetID()) {
                        continue;
                    }
                    break;
                }
                case OpCode::Phi: {
                    auto* phi = instruction->As<PhiInstruction>();

                    // Patch relevant value
                    for (uint32_t i = 0; i < phi->values.count; i++) {
                        const PhiValue& value = phi->values[i];

                        // Only care about the operand referencing this block
                        if (value.branch != GetID()) {
                            continue;
                        }

#if 0
                        // Referenced value, originates from *this* block
                        OpaqueInstructionRef phiValueRef = map.Get(value.value);

                        // Is the referenced value beyond the split point?
                        bool isPostSplitPoint = phiValueRef.relocationOffset->offset >= splitPointRelocationOffset;
                        if (!isPostSplitPoint) {
                            continue;
                        }
#endif

                        // Set new owning block
                        phi->values[i].branch = destBlock->GetID();
                    }
                    break;
                }
            }

            // Add new block user
            map.AddBlockUser(destBlock->GetID(), ref);

            // Mark user instruction as dirty
            instruction->source = instruction->source.Modify();

            // Mark the branch block as dirty to ensure recompilation
            ref.basicBlock->MarkAsDirty();

            // Latent removal
            removed.Add(ref);
        }

        // Remove references
        for (const IL::OpaqueInstructionRef& ref : removed) {
            map.RemoveBlockUser(GetID(), ref);
        }
    }

    // Append all instructions after the split point to the new basic block
    Iterator moveIterator = splitIteratorPhi;
    for (; moveIterator != end(); moveIterator++) {
        destBlock->Append(moveIterator.Get());
        count--;
    }

    // Erase moved instruction data
    data.erase(data.begin() + splitPointRelocationOffset, data.end());

    // Free relocation indices
    auto relocationIt = relocationTable.begin() + splitIteratorPhi.relocationIndex;
    for (auto it = relocationIt; it != relocationTable.end(); it++) {
        relocationAllocator.Free(*it);
    }

    // Erase relocation indices
    relocationTable.erase(relocationIt, relocationTable.end());

    // Do we have back edge phi operations we need to resolve?
    if (hasUnresolvedBackEdgePhis) {
        BranchControlFlow controlFlow;
        Backend::IL::GetControlFlow(destBlock->GetTerminator(), controlFlow);
            
        // Find all moved phi's
        for (auto it = destBlock->begin(); it != destBlock->end() && it->Is<PhiInstruction>(); ++it) {
            auto* instr = it.GetMutable()->As<PhiInstruction>();
            
            // If there's just two dependencies, it's on the immediate predecessor and the back edge
            // No need to split the phi!
            if (instr->values.count == 2u) {
                // Incoming branch is now the split predecessor
                for (uint32_t i = 0; i < 2u; i++) {
                    PhiValue& value = instr->values[i];
                    if (value.branch != controlFlow._continue) {
                        value.branch = GetID();
                    }
                }
                
                continue;
            }

            // This is a complex phi that needs to be split
            // Phi A resolves the incoming source edges
            auto phiA = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(instr->values.count - 1));
            phiA->opCode = OpCode::Phi;
            phiA->source = Source::Invalid();
            phiA->values.count = 0;

            // This is not user mapped!
            phiA->result = map.AllocID();

            // Copy non-backedge values
            PhiValue backEdgePhiValue{InvalidID, InvalidID};
            for (uint32_t i = 0; i < instr->values.count; i++) {
                PhiValue value = instr->values[i];
                
                if (value.branch == controlFlow._continue) {
                    backEdgePhiValue = value;
                    continue;
                }
                
                phiA->values[phiA->values.count++] = value;
            }

            // Append to predecessor block
            ASSERT(phiA->values.count == instr->values.count - 1, "Invalid phi value count");
            Append(phiA);

            // Phi B resolves the incoming back edge and immediate predecessor
            auto phiB = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(2u));
            phiB->opCode = OpCode::Phi;
            phiB->source = Source::Invalid();
            phiB->result = instr->result;
            phiB->values.count = 2u;

            // First edge is the previous partial resolve
            phiB->values[0] = PhiValue {
                .value = phiA->result,
                .branch = GetID()
            };

            // Second edge is the original backedge
            ASSERT(backEdgePhiValue.branch != InvalidID, "Invalid backedge migration");
            phiB->values[1] = backEdgePhiValue;

            // Replace the resolved instruction
            it = Replace(it, *phiB);
        }
    }

    // Return first iterator
    return destBlock->begin();
}
