#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockDebugStringSource.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

void SpvPhysicalBlockDebugStringSource::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);

    // Set entry count
    debugMap.SetBound(table.scan.header.bound);
    sourceMap.SetBound(table.scan.header.bound);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpSource: {
                auto language = static_cast<SpvSourceLanguage>(ctx++);
                uint32_t version = ctx++;

                // Optional filename
                uint32_t fileId{InvalidSpvId};
                if (ctx.HasPendingWords()) {
                    fileId = ctx++;
                    sourceMap.AddPhysicalSource(fileId, language, version, debugMap.Get(fileId, SpvOpString));
                }

                // Optional fragment?
                if (ctx.HasPendingWords()) {
                    sourceMap.AddSource(fileId, std::string_view(reinterpret_cast<const char*>(ctx.GetInstructionCode()), (ctx->GetWordCount() - 4) * sizeof(uint32_t)));
                }
                break;
            }
            case SpvOpSourceContinued: {
                sourceMap.AddSource(sourceMap.GetPendingSource(), std::string_view(reinterpret_cast<const char*>(ctx.GetInstructionCode()), (ctx->GetWordCount() - 1) * sizeof(uint32_t)));
                break;
            }
            case SpvOpString: {
                debugMap.Add(ctx.GetResult(), SpvOpString, std::string_view(reinterpret_cast<const char*>(ctx.GetInstructionCode()), (ctx->GetWordCount() - 2) * sizeof(uint32_t)));
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockDebugStringSource::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockDebugStringSource &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);
    out.debugMap = debugMap;
    out.sourceMap = sourceMap;
}
