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

// Backend
#include <Backend/IL/Metadata/DataMetadata.h>

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
                auto decorationKind = static_cast<SpvDecoration>(ctx++);
                switch (decorationKind) {
                    default: {
                        // Parse pair
                        SpvDecorationPair kv;
                        kv.kind = decorationKind;
                        kv.wordCount = ctx.PendingWords();

                        // Copy payload data
                        if (kv.wordCount) {
                            kv.words = table.recordAllocator.AllocateArray<uint32_t>(kv.wordCount);
                            std::memcpy(kv.words, ctx.GetInstructionCode(), sizeof(uint32_t) * kv.wordCount);
                        }
                
                        decoration.value.decorations.push_back(kv);
                        break;
                    }
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
                    case SpvDecorationBuiltIn: {
                        builtinMap[static_cast<SpvBuiltIn>(ctx++)] = target;
                        break;
                    }
                    case SpvDecorationNonUniform: {
                        program.GetMetadataMap().AddMetadata(target, IL::MetadataType::DivergentResourceIndex);
                        break;
                    }
                    case SpvDecorationArrayStride: {
                        program.GetMetadataMap().AddMetadata(target, IL::ArrayStrideMetadata {
                            .byteStride = ctx++
                        });
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
                if (member >= decoration.value.members.size()) {
                    decoration.value.members.resize(member + 1);
                }

                // Append decoration
                SpvMemberDecoration& memberDecoration = decoration.value.members[member];

                // Parse pair
                SpvDecorationPair kv;
                kv.kind = static_cast<SpvDecoration>(ctx++);
                kv.wordCount = ctx.PendingWords();

                // Copy payload data
                if (kv.wordCount) {
                    kv.words = table.recordAllocator.AllocateArray<uint32_t>(kv.wordCount);
                    std::memcpy(kv.words, ctx.GetInstructionCode(), sizeof(uint32_t) * kv.wordCount);
                }
                
                // Handle decoration
                switch (kv.kind) {
                    default: {
                        break;
                    }
                    case SpvDecorationOffset: {
                        program.GetMetadataMap().AddMetadata(target, member, IL::OffsetMetadata {
                            .byteOffset = kv.words[0]
                        });
                        break;
                    }
                }
                
                memberDecoration.decorations.push_back(kv);
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
