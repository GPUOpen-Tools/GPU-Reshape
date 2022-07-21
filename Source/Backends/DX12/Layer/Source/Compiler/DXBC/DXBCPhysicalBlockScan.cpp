#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXStream.h>

DXBCPhysicalBlockScan::DXBCPhysicalBlockScan(const Allocators &allocators) : allocators(allocators) {

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
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Capabilities):
            return DXBCPhysicalBlockType::Capabilities;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Shader4):
            return DXBCPhysicalBlockType::Shader4;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Shader5):
            return DXBCPhysicalBlockType::Shader5;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ILDB):
            return DXBCPhysicalBlockType::ILDB;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::DXIL):
            return DXBCPhysicalBlockType::DXIL;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::ShaderDebug1):
            return DXBCPhysicalBlockType::ShaderDebug1;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Statistics):
            return DXBCPhysicalBlockType::Statistics;
        case static_cast<uint32_t>(DXBCPhysicalBlockType::Unexposed):
            return DXBCPhysicalBlockType::Unexposed;
    }
}

bool DXBCPhysicalBlockScan::Scan(const void* byteCode, uint64_t byteLength) {
    DXBCParseContext ctx(byteCode, byteLength);

    // Consume header
    header = ctx.Consume<DXBCHeader>();

    // Must be DXBC
    if (header.identifier != 'CBXD') {
        return false;
    }

    // Preallocate
    sections.resize(header.chunkCount);

    // Populate chunks
    for (uint32_t chunkIndex = 0; chunkIndex < header.chunkCount; chunkIndex++) {
        auto chunk = ctx.Consume<DXBCChunkEntryHeader>();

        // Header of the chunk
        auto chunkHeader = ctx.ReadAt<DXBCChunkHeader>(chunk.offset);

        // Configure section
        Section &section = sections[chunkIndex];
        section.type = FilterChunkType(chunkHeader->type);
        section.block.ptr = ctx.ReadAt<uint8_t>(chunk.offset);
        section.block.length = chunkHeader->size;
    }

    // OK
    return true;
}

void DXBCPhysicalBlockScan::Stitch(DXStream &out) {
    for (const Section& section : sections) {
        if (section.block.stream.GetByteSize()) {
            out.AppendData(section.block.stream.GetData(), section.block.stream.GetByteSize());
        } else {
            out.AppendData(section.block.ptr, section.block.length);
        }
    }
}

void DXBCPhysicalBlockScan::CopyTo(DXBCPhysicalBlockScan& out) {
    out.sections.insert(out.sections.end(), sections.begin(), sections.end());
}
