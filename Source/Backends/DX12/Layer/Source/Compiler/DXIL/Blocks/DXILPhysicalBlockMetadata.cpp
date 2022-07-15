#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockMetadata.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockMetadata::ParseMetadata(const struct LLVMBlock *block) {
    // Empty out previous block data
    metadata.clear();
    metadata.resize(block->records.Size());

    // Current name
    LLVMRecordStringView recordName;

    // Visit records
    for (size_t i = 0; i < block->records.Size(); i++) {
        const LLVMRecord &record = block->records[i];

        // Get metadata
        Metadata &md = metadata[i];
        md.record = &record;

        // Handle record
        switch (static_cast<LLVMMetadataRecord>(record.id)) {
            default: {
                // Ignored
                break;
            }

            case LLVMMetadataRecord::Name: {
                // Set name
                recordName = LLVMRecordStringView(record, 0);

                // Validate next
                ASSERT(i + 1 != block->records.Size(), "Expected succeeding metadata record");
                ASSERT(block->records[i + 1].Is(LLVMMetadataRecord::NamedNode), "Succeeding record to Name must be NamedNode");
                break;
            }

                // String name?
            case LLVMMetadataRecord::StringOld: {
                md.name.resize(record.opCount);
                record.FillOperands(md.name.data());
                break;
            }

                // Named successor
            case LLVMMetadataRecord::NamedNode: {
                ParseNamedNode(block, record, recordName);
                break;
            }

                // Constant value
            case LLVMMetadataRecord::Value: {
                if (table.idMap.GetType(record.Op(1)) == DXILIDType::Constant) {
                    auto mapped = table.idMap.GetMappedCheckType(record.Op(1), DXILIDType::Constant);
                    md.value.type = table.type.typeMap.GetType(record.Op(0));
                    md.value.constant = program.GetConstants().GetConstant(mapped);
                    ASSERT(md.value.type, "Expected type");
                    ASSERT(md.value.constant, "Expected constant");
                }
                break;
            }
        }
    }
}

void DXILPhysicalBlockMetadata::ParseNamedNode(const struct LLVMBlock *block, const LLVMRecord &record, const LLVMRecordStringView &name) {
    switch (name.GetHash()) {
        // Resource declaration record
        case CRC64("dx.resources"): {
            if (name != "dx.resources") {
                return;
            }

            // Get list
            ASSERT(record.opCount == 1, "Expected a single value for dx.resources");
            const LLVMRecord &list = block->records[record.Op(0)];

            // Check SRVs
            if (resources.srvs = list.Op(static_cast<uint32_t>(DXILShaderResourceClass::SRVs)); resources.srvs != 0) {
                ParseResourceList(block, DXILShaderResourceClass::SRVs, resources.srvs);
            }

            // Check UAVs
            if (resources.uavs = list.Op(static_cast<uint32_t>(DXILShaderResourceClass::UAVs)); resources.uavs != 0) {
                ParseResourceList(block, DXILShaderResourceClass::UAVs, resources.uavs);
            }

            // Check CBVs
            if (resources.cbvs = list.Op(static_cast<uint32_t>(DXILShaderResourceClass::CBVs)); resources.cbvs != 0) {
                ParseResourceList(block, DXILShaderResourceClass::CBVs, resources.cbvs);
            }

            // Check samplers
            if (resources.samplers = list.Op(static_cast<uint32_t>(DXILShaderResourceClass::Samplers)); resources.samplers != 0) {
                ParseResourceList(block, DXILShaderResourceClass::Samplers, resources.samplers);
            }

            break;
        }
    }
}

void DXILPhysicalBlockMetadata::ParseResourceList(const struct LLVMBlock *block, DXILShaderResourceClass type, uint32_t id) {
    const LLVMRecord &record = block->records[id - 1];

    // For each resource
    for (uint32_t i = 0; i < record.opCount; i++) {
        const LLVMRecord &resource = block->records[record.ops[i] - 1];

        // Prepare entry
        HandleEntry entry;
        entry._class = type;
        entry.record = &resource;

        // Get handle i
        uint64_t resourceID = GetOperandConstant<IL::IntConstant>(resource.Op(0))->value;

        // Undef constant
        const IL::Constant *constantPointer = GetOperandConstant(resource.Op(1));

        // TODO: How on earth are the names stored?
        // uint64_t name = GetOperandConstant<IL::IntConstant>(resource.Op(2))->value;

        // Resource binding
        uint64_t bindSpace = GetOperandConstant<IL::IntConstant>(resource.Op(3))->value;
        uint64_t rsBase = GetOperandConstant<IL::IntConstant>(resource.Op(4))->value;
        uint64_t rsRange = GetOperandConstant<IL::IntConstant>(resource.Op(5))->value;

        // Handle based on type
        switch (entry._class) {
            default:
                ASSERT(false, "Invalid resource type");
                break;
            case DXILShaderResourceClass::SRVs: {
                // Unique ops
                auto shape = resource.OpAs<DXILShaderResourceShape>(6);
                uint64_t sampleCount = GetOperandConstant<IL::IntConstant>(resource.Op(7))->value;

                // Get extended metadata
                Metadata &extendedMetadata = metadata[resource.Op(8) - 1];
                ASSERT(extendedMetadata.record->opCount == 2, "Expected 2 operands for extended metadata");

                // Buffer shape?
                if (IsBuffer(shape)) {
                    Backend::IL::BufferType buffer{};
                    buffer.samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(buffer);
                } else {
                    Backend::IL::TextureType texture{};
                    texture.samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(texture);
                }
                break;
            }
            case DXILShaderResourceClass::UAVs: {
                // Unique ops
                auto shape = static_cast<DXILShaderResourceShape>(GetOperandConstant<IL::IntConstant>(resource.Op(6))->value);
                uint64_t globallyCoherent = GetOperandConstant<IL::IntConstant>(resource.Op(7))->value;
                uint64_t counter = GetOperandConstant<IL::IntConstant>(resource.Op(8))->value;
                uint64_t rasterizerOrderedView = GetOperandConstant<IL::IntConstant>(resource.Op(9))->value;

                // Get extended metadata
                Metadata &extendedMetadata = metadata[resource.Op(10) - 1];
                ASSERT(extendedMetadata.record->opCount == 2, "Expected 2 operands for extended metadata");

                // Buffer shape?
                if (IsBuffer(shape)) {
                    Backend::IL::BufferType buffer{};
                    buffer.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(buffer);
                } else {
                    Backend::IL::TextureType texture{};
                    texture.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(texture);
                }
                break;
            }
            case DXILShaderResourceClass::CBVs: {
                // Unique ops
                uint64_t byteSize = GetOperandConstant<IL::IntConstant>(resource.Op(6))->value;
                uint64_t extendedMetadata = GetOperandConstant<IL::IntConstant>(resource.Op(7))->value;
                break;
            }
            case DXILShaderResourceClass::Samplers: {
                // Unique ops
                uint64_t samplerType = GetOperandConstant<IL::IntConstant>(resource.Op(6))->value;
                uint64_t extendedMetadata = GetOperandConstant<IL::IntConstant>(resource.Op(7))->value;
                break;
            }
        }

        // Ensure space
        if (handleEntries.size() <= resourceID) {
            handleEntries.resize(resourceID + 1);
        }

        // Set entry at id
        handleEntries[resourceID] = entry;
    }
}

const Backend::IL::Type *DXILPhysicalBlockMetadata::GetHandleType(uint32_t handleID) {
    if (handleEntries.size() <= handleID || !handleEntries[handleID].record) {
        return nullptr;
    }

    // Get entry
    return handleEntries[handleID].type;
}
