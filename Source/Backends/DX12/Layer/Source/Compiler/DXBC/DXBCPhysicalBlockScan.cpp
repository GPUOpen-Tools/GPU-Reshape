// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Compiler/DXStream.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>
#include <Backends/DX12/Config.h>

// Special includes
#if DXBC_DUMP_STREAM
// Common
#   include <Common/FileSystem.h>

// Std
#   include <fstream>
#endif // DXBC_DUMP_STREAM

DXBCPhysicalBlockScan::DXBCPhysicalBlockScan(const Allocators &allocators) : sections(allocators), allocators(allocators) {

}

/// Filter (whitelist) the chunk type
static DXBCPhysicalBlockType FilterChunkType(uint32_t type) {
    switch (type) {
        default:
            return DXBCPhysicalBlockType::Unexposed;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Interface):
            return DXBCPhysicalBlockType::Interface;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Input):
            return DXBCPhysicalBlockType::Input;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Output5):
            return DXBCPhysicalBlockType::Output5;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Output):
            return DXBCPhysicalBlockType::Output;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Patch):
            return DXBCPhysicalBlockType::Patch;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Resource):
            return DXBCPhysicalBlockType::Resource;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ShaderDebug0):
            return DXBCPhysicalBlockType::ShaderDebug0;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::FeatureInfo):
            return DXBCPhysicalBlockType::FeatureInfo;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Shader4):
            return DXBCPhysicalBlockType::Shader4;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Shader5):
            return DXBCPhysicalBlockType::Shader5;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ShaderHash):
            return DXBCPhysicalBlockType::ShaderHash;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ILDB):
            return DXBCPhysicalBlockType::ILDB;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ILDN):
            return DXBCPhysicalBlockType::ILDN;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::DXIL):
            return DXBCPhysicalBlockType::DXIL;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ShaderDebug1):
            return DXBCPhysicalBlockType::ShaderDebug1;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Statistics):
            return DXBCPhysicalBlockType::Statistics;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::PipelineStateValidation):
            return DXBCPhysicalBlockType::PipelineStateValidation;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::RootSignature):
            return DXBCPhysicalBlockType::RootSignature;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::InputSignature):
            return DXBCPhysicalBlockType::InputSignature;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::OutputSignature):
            return DXBCPhysicalBlockType::OutputSignature;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Unexposed):
            return DXBCPhysicalBlockType::Unexposed;
    }
}

bool DXBCPhysicalBlockScan::Scan(const void* byteCode, uint64_t byteLength) {
    DXBCParseContext ctx(byteCode, byteLength);

    // Dump?
#if DXBC_DUMP_STREAM
    // Write stream to immediate path
    std::ofstream out(GetIntermediateDebugPath() / "scan.dxbc", std::ios_base::binary);
    out.write(reinterpret_cast<const char *>(byteCode), byteLength);
    out.close();
#endif // DXBC_DUMP_STREAM

    // Consume header
    header = ctx.Consume<DXBCHeader>();

    // Must be DXBC
    if (header.identifier != 'CBXD') {
        return false;
    }

    // Preallocate
    sections.resize(header.chunkCount, allocators);

    // Populate chunks
    for (uint32_t chunkIndex = 0; chunkIndex < header.chunkCount; chunkIndex++) {
        auto chunk = ctx.Consume<DXBCChunkEntryHeader>();

        // Header of the chunk
        auto chunkHeader = ctx.ReadAt<DXBCChunkHeader>(chunk.offset);

        // Header offset
        const uint32_t headerOffset = sizeof(DXBCChunkHeader);

        // Configure section
        Section &section = sections[chunkIndex];
        section.unexposedType = chunkHeader->type;
        section.type = FilterChunkType(chunkHeader->type);
        section.block.ptr = ctx.ReadAt<uint8_t>(chunk.offset + headerOffset);
        section.block.length = chunkHeader->size;
    }

    // OK
    return true;
}

void DXBCPhysicalBlockScan::Stitch(const DXCompileJob& job, DXStream &out, bool sign) {
    // Write header out
    uint64_t headerOffset = out.Append(header);

    // Starting chunk offset
    uint32_t chunkOffset = out.GetByteSize() + static_cast<uint32_t>(sizeof(DXBCChunkEntryHeader) * sections.size());

    // Write section entries
    for (const Section& section : sections) {
        DXBCChunkEntryHeader entry;
        entry.offset = chunkOffset;
        out.Append(entry);

        // Determine size
        uint32_t chunkSize;
        if (section.block.stream.GetByteSize()) {
            chunkSize = static_cast<uint32_t>(section.block.stream.GetByteSize());
        } else {
            chunkSize = section.block.length;
        }

        // Next
        chunkOffset += sizeof(DXBCChunkHeader) + chunkSize;
    }

    // Write all sections
    for (const Section& section : sections) {
        // Determine size
        uint32_t chunkSize;
        if (section.block.stream.GetByteSize()) {
            chunkSize = static_cast<uint32_t>(section.block.stream.GetByteSize());
        } else {
            chunkSize = section.block.length;
        }

        // Write chunk header
        DXBCChunkHeader chunkHeader;
        chunkHeader.type = section.unexposedType;
        chunkHeader.size = chunkSize;
        out.Append(chunkHeader);

        // Write chunk contents
        if (section.block.stream.GetByteSize()) {
            out.AppendData(section.block.stream.GetData(), section.block.stream.GetByteSize());
        } else {
            out.AppendData(section.block.ptr, section.block.length);
        }
    }

    // End offset
    uint64_t endOffset = out.GetOffset();

    // Total length
    uint64_t byteLength = endOffset - headerOffset;

    // Patch the header
    auto* stitchHeader = out.GetMutableDataAt<DXBCHeader>(headerOffset);
    std::memset(stitchHeader->privateChecksum, 0x0, sizeof(stitchHeader->privateChecksum));
    stitchHeader->byteCount = static_cast<uint32_t>(byteLength);

    // Dump?
#if DXBC_DUMP_STREAM
    // Write stream to immediate path
    std::ofstream outStream(GetIntermediateDebugPath() / "stitch.dxbc", std::ios_base::binary);
    outStream.write(reinterpret_cast<const char *>(out.GetMutableData()), out.GetByteSize());
    outStream.close();
#endif // DXBC_DUMP_STREAM

    // No signing?
    if (!sign) {
        return;
    }

    // Finally, sign the resulting byte-code using the official signers
    if (GetPhysicalBlock(DXBCPhysicalBlockType::DXIL)) {
        bool result = job.dxilSigner->Sign(out.GetMutableDataAt(headerOffset), byteLength);
        ASSERT(result, "Failed to sign DXIL");
    } else {
        bool result = job.dxbcSigner->Sign(out.GetMutableDataAt(headerOffset), byteLength);
        ASSERT(result, "Failed to sign DXBC");
    }
}

void DXBCPhysicalBlockScan::CopyTo(DXBCPhysicalBlockScan& out) {
    out.header = header;
    out.sections.insert(out.sections.end(), sections.begin(), sections.end());
}
