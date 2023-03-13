#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockAnnotation.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

void SpvPhysicalBlockAnnotation::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::Annotation);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpDecorate: {
                uint32_t target = ctx++;

                // Allocate if needed
                if (target >= entries.size()) {
                    entries.resize(target + 1);
                }

                // Get decoration
                SpvDecorationEntry& decoration = entries[target];
                decoration.decorated = true;

                // Handle decoration
                switch (static_cast<SpvDecoration>(ctx++)) {
                    default:
                        break;
                    case SpvDecorationDescriptorSet: {
                        decoration.value.descriptorSet = ctx++;

                        // Set limit
                        boundDescriptorSets = std::max(boundDescriptorSets, decoration.value.descriptorSet);
                        break;
                    }
                    case SpvDecorationBinding: {
                        decoration.value.descriptorOffset = ctx++;
                        break;
                    }
                }
                break;
            }
            case SpvOpMemberDecorate: {
                uint32_t target = ctx++;
                uint32_t member = ctx++;

                // Allocate if needed
                if (target >= entries.size()) {
                    entries.resize(target + 1);
                }

                // Get decoration
                SpvDecorationEntry& decoration = entries[target];
                decoration.decorated = true;

                // Allocate if needed
                if (member >= decoration.value.memberDecorations.size()) {
                    decoration.value.memberDecorations.resize(member + 1);
                }

                // Get decoration for member
                SpvValueDecoration& valueDecoration = decoration.value.memberDecorations[member];

                // Handle decoration
                switch (static_cast<SpvDecoration>(ctx++)) {
                    default:
                        break;
                    case SpvDecorationOffset: {
                        valueDecoration.blockOffset = ctx++;
                        break;
                    }
                }
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockAnnotation::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockAnnotation &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::Annotation);
    out.boundDescriptorSets = boundDescriptorSets;
    out.entries = entries;
}
