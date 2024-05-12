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

#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockShaderSourceInfo.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

// Common
#include <Common/String.h>

// ZLIB
#include <zlib.h>

DXBCPhysicalBlockShaderSourceInfo::DXBCPhysicalBlockShaderSourceInfo(DXBCPhysicalBlockScan &scan) : scan(scan) {
    
}

void DXBCPhysicalBlockShaderSourceInfo::Parse() {
    // Block is optional
    DXBCPhysicalBlock* block = scan.GetPhysicalBlock(DXBCPhysicalBlockType::ShaderSourceInfo);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Read header
    auto info = ctx.Consume<DXILSourceInfo>();

    // Handle all sections
    for (uint32_t i = 0; i < info.sectionCount; i++) {
        size_t start = ctx.Offset();

        // Handle section
        auto section = ctx.Consume<DXILSourceInfoSection>();
        switch (section.type) {
            default: {
                ASSERT(false, "Invalid secion type");
                break;
            }
            case DXILSourceInfoSectionType::SourceContents: {
                auto contents = ctx.Consume<DXILSourceInfoSourceContents>();

                // Handle decompression
                DXBCParseContext contentsCtx(nullptr, 0);
                switch (contents.compressType) {
                    default: {
                        ASSERT(false, "Invalid compression type");
                        break;
                    }
                    case DXILSourceInfoSourceContentsCompressType::None: {
                        // No compression, just keep the context
                        contentsCtx = ctx;
                        break;
                    }
                    case DXILSourceInfoSourceContentsCompressType::ZLib: {
                        decompressionBlob.resize(contents.uncompressedEntriesByteSize);

                        // Decompress in one go as we know the decompressed size
                        unsigned long length = contents.uncompressedEntriesByteSize;
                        if (uncompress(decompressionBlob.data(), &length, &ctx.Get<Bytef>(), contents.entriesByteSize) != Z_OK) {
                            ASSERT(false, "Failed decompression, zlib faulted");
                            break;
                        }

                        // Validate actual decompressed size matches
                        if (length != contents.uncompressedEntriesByteSize) {
                            ASSERT(false, "Failed decompression, unexpected size");
                            break;
                        }

                        // Assume blob as context
                        contentsCtx = DXBCParseContext(decompressionBlob.data(), decompressionBlob.size());
                        break;
                    }
                }

                // Parse all entries
                for (uint32_t contentIndex = 0; contentIndex < contents.count; contentIndex++) {
                    size_t contentOffset = contentsCtx.Offset();

                    // Read entry
                    auto contentEntry = contentsCtx.Consume<DXILSourceInfoSourceContentsEntry>();

                    // Set file contents
                    sourceFiles.at(contentIndex).contents = std::string_view(&contentsCtx.Get<const char>(), contentEntry.contentByteSize - 1);

                    // Aligned next
                    contentsCtx.SetOffset(contentOffset + contentEntry.alignedByteSize);
                }
                break;
            }
            case DXILSourceInfoSectionType::SourceNames: {
                auto names = ctx.Consume<DXILSourceInfoSourceNames>();

                // Source names always appears before source contents, preallocate
                sourceFiles.resize(names.count);

                // Parse names
                for (uint32_t nameIndex = 0; nameIndex < names.count; nameIndex++) {
                    size_t nameOffset = ctx.Offset();

                    // Read entry
                    auto nameEntry = ctx.Consume<DXILSourceInfoSourceNamesEntry>();

                    // Set filename
                    sourceFiles.at(nameIndex).filename = std::string_view(&ctx.Get<const char>(), nameEntry.nameByteSize - 1);

                    // Aligned next
                    ctx.SetOffset(nameOffset + nameEntry.alignedByteSize);
                }
                break;
            }
            case DXILSourceInfoSectionType::Args: {
                auto args = ctx.Consume<DXILSourceInfoArgs>();

                // Argument iterators
                const auto* str = &ctx.Get<const char>();
                const auto* end = &ctx.Get<const char>() + args.byteSize;

                // While we have pending arguments
                while (str < end) {
                    SourceArg arg;

                    // Read name until terminator
                    arg.name = std::string_view(str, std::strchr(str, '\0') - str);
                    str += arg.name.length() + 1u;

                    // Read value until terminator
                    arg.value = std::string_view (str, std::strchr(str, '\0') - str);
                    str += arg.value.length() + 1u;

                    // If either is present, there's something to add
                    if (!arg.name.empty() || !arg.value.empty()) {
                        sourceArgs.push_back(arg);
                    }
                }
                
                break;
            }
        }

        // Aligned next
        ctx.SetOffset(start + section.alignedByteSize);
    }
}

bool DXBCPhysicalBlockShaderSourceInfo::IsSlimPDB() {
    for (const SourceArg& arg : sourceArgs) {
        if (std::iequals(std::string(arg.name), "Zs")) {
            return true;
        }
    }

    // None found
    return false;
}
