// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
