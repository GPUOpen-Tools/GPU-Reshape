#include <Backend/IL/BasicBlock.h>

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
        auto result = it->result;

        // Anything to index?
        if (result != InvalidID) {
            map.AddInstruction(it.Ref(), result);
        }

        // Handle users in instructions
        switch (it->opCode) {
            default:
                break;
            case OpCode::Phi: {
                auto* phi = it->As<PhiInstruction>();

                for (uint32_t i = 0; i < phi->values.count; i++) {
                    const IL::PhiValue& value = phi->values[i];
                    map.AddBlockUser(value.branch, it);
                }
                break;
            }
        }
    }
}

IL::BasicBlock::Iterator IL::BasicBlock::Split(IL::BasicBlock *destBlock, const IL::BasicBlock::Iterator &splitIterator, BasicBlockSplitFlagSet splitFlags) {
    ASSERT(destBlock->IsEmpty(), "Cannot split into a filled basic block");

    // Append all instructions after the split point to the new basic block
    Iterator moveIterator = splitIterator;
    for (; moveIterator != end(); moveIterator++) {
        destBlock->Append(moveIterator.Get());
        count--;
    }

    // Erase moved instruction data
    data.erase(data.begin() + relocationTable[splitIterator.relocationIndex]->offset, data.end());

    // Free relocation indices
    auto relocationIt = relocationTable.begin() + splitIterator.relocationIndex;
    for (auto it = relocationIt; it != relocationTable.end(); it++) {
        relocationAllocator.Free(*it);
    }

    // Erase relocation indices
    relocationTable.erase(relocationIt, relocationTable.end());

    // Redirect all phi users if requested
    if (splitFlags & BasicBlockSplitFlag::RedirectPhiUsers) {
        for (const OpaqueInstructionRef& ref : map.GetBlockUsers(GetID())) {
            // Check for phi
            auto* phi = ref.basicBlock->GetRelocationInstruction(ref.relocationOffset)->Cast<PhiInstruction>();
            if (!phi) {
                continue;
            }

            // Patch relevant value
            for (uint32_t i = 0; i < phi->values.count; i++) {
                // Only care about the operand referencing this block
                if (phi->values[i].branch != GetID()) {
                    continue;
                }

                // Referenced value, originates from *this* block
                OpaqueInstructionRef phiValueRef = map.Get(phi->values[i].value);

                // Is the referenced value beyond the split point?
                bool isPostSplitPoint = phiValueRef.relocationOffset->offset >= data.size();
                if (!isPostSplitPoint) {
                    continue;
                }

                // Set new owning block
                phi->values[i].branch = destBlock->GetID();
            }

            // Mark phi instruction as dirty
            phi->source = phi->source.Modify();

            // Mark the phi block as dirty to ensure recompilation
            ref.basicBlock->MarkAsDirty();
        }
    }

    // Return first iterator
    return destBlock->begin();
}
