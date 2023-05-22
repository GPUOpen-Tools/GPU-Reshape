#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockDebugStringSource.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

SpvPhysicalBlockDebugStringSource::SpvPhysicalBlockDebugStringSource(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable &table)
    : SpvPhysicalBlockSection(allocators, program, table),
      sourceMap(allocators) {
    
}

void SpvPhysicalBlockDebugStringSource::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);

    // Set entry count
    debugMap.SetBound(table.scan.header.bound);

    // Current source being processed
    uint32_t pendingSource{};

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

                    // Set pending
                    pendingSource = fileId;
                }

                // Optional fragment?
                if (ctx.HasPendingWords()) {
                    auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                    uint32_t length = (ctx->GetWordCount() - 4) * sizeof(uint32_t);
                    while (!ptr[length - 1]) {
                        length--;
                    }

                    sourceMap.AddSource(fileId, std::string_view(ptr, length));
                }
                break;
            }
            case SpvOpSourceContinued: {
                auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                uint32_t length = (ctx->GetWordCount() - 1) * sizeof(uint32_t);
                while (!ptr[length - 1]) {
                    length--;
                }

                sourceMap.AddSource(pendingSource, std::string_view(ptr, length));
                break;
            }
            case SpvOpString: {
                auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                uint32_t length = (ctx->GetWordCount() - 2) * sizeof(uint32_t);
                while (!ptr[length - 1]) {
                    length--;
                }

                debugMap.Add(ctx.GetResult(), SpvOpString, std::string_view(ptr, length));
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }

    // Finalize all sources
    sourceMap.Finalize();
}

void SpvPhysicalBlockDebugStringSource::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockDebugStringSource &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);
    out.debugMap = debugMap;
    out.sourceMap = sourceMap;
}
