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
                auto decoration = static_cast<SpvDecoration>(ctx++);

                // Handle decoration
                switch (decoration) {
                    default:
                        break;
                    case SpvDecorationDescriptorSet: {
                        boundDescriptorSets = std::max(boundDescriptorSets, ctx++);
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
}
