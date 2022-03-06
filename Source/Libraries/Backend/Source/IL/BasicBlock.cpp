#include <Backend/IL/BasicBlock.h>

// Common
#include <Common/Alloca.h>

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

    // Byte offset to the split point
    uint32_t splitPointRelocationOffset = relocationTable[splitIterator.relocationIndex]->offset;

    // Redirect all phi users if requested
    if (splitFlags & BasicBlockSplitFlag::RedirectPhiUsers) {
        const IdentifierMap::BlockUserList& users = map.GetBlockUsers(GetID());

        auto* removed = ALLOCA_ARRAY(OpaqueInstructionRef, users.size());
        uint32_t removedCount{0};

        for (const OpaqueInstructionRef& ref : users) {
            // Check for phi
            auto* phi = ref.basicBlock->GetRelocationInstruction(ref.relocationOffset)->Cast<PhiInstruction>();
            if (!phi) {
                continue;
            }

            // Patch relevant value
            for (uint32_t i = 0; i < phi->values.count; i++) {
                const PhiValue& value = phi->values[i];

                // Only care about the operand referencing this block
                if (value.branch != GetID()) {
                    continue;
                }

                // Referenced value, originates from *this* block
                OpaqueInstructionRef phiValueRef = map.Get(value.value);

#if 0
                // Is the referenced value beyond the split point?
                bool isPostSplitPoint = phiValueRef.relocationOffset->offset >= splitPointRelocationOffset;
                if (!isPostSplitPoint) {
                    continue;
                }
#endif

                // Set new owning block
                phi->values[i].branch = destBlock->GetID();
            }

            // Mark phi instruction as dirty
            phi->source = phi->source.Modify();

            // Mark the phi block as dirty to ensure recompilation
            ref.basicBlock->MarkAsDirty();

            // Add reference to the new block
            map.AddBlockUser(destBlock->GetID(), ref);

            // Latent removal
            removed[removedCount++] = ref;
        }

        // Remove references
        for (uint32_t i = 0; i < removedCount; i++) {
            map.RemoveBlockUser(GetID(), removed[i]);
        }
    }

    // Append all instructions after the split point to the new basic block
    Iterator moveIterator = splitIterator;
    for (; moveIterator != end(); moveIterator++) {
        destBlock->Append(moveIterator.Get());
        count--;
    }

    // Erase moved instruction data
    data.erase(data.begin() + splitPointRelocationOffset, data.end());

    // Free relocation indices
    auto relocationIt = relocationTable.begin() + splitIterator.relocationIndex;
    for (auto it = relocationIt; it != relocationTable.end(); it++) {
        relocationAllocator.Free(*it);
    }

    // Erase relocation indices
    relocationTable.erase(relocationIt, relocationTable.end());

    // Return first iterator
    return destBlock->begin();
}
