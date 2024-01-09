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
