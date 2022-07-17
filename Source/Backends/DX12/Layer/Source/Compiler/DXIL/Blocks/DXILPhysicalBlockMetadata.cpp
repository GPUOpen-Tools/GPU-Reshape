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
        uint64_t resourceID = GetOperandU32Constant(resource.Op(0));

        // Undef constant
        const IL::Constant *constantPointer = GetOperandConstant(resource.Op(1));

        // TODO: How on earth are the names stored?
        // uint64_t name = GetOperandU32Constant(resource.Op(2))->value;

        // Resource binding
        uint64_t bindSpace = GetOperandU32Constant(resource.Op(3));
        uint64_t rsBase = GetOperandU32Constant(resource.Op(4));
        uint64_t rsRange = GetOperandU32Constant(resource.Op(5));

        // Handle based on type
        switch (entry._class) {
            default:
            ASSERT(false, "Invalid resource type");
                break;
            case DXILShaderResourceClass::SRVs: {
                // Unique ops
                auto shape = resource.OpAs<DXILShaderResourceShape>(6);
                uint64_t sampleCount = GetOperandU32Constant(resource.Op(7));

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
                auto shape = static_cast<DXILShaderResourceShape>(GetOperandU32Constant(resource.Op(6)));
                uint64_t globallyCoherent = GetOperandU32Constant(resource.Op(7));
                uint64_t counter = GetOperandU32Constant(resource.Op(8));
                uint64_t rasterizerOrderedView = GetOperandU32Constant(resource.Op(9));

                // Get extended metadata
                Metadata &extendedMetadata = metadata[resource.Op(10) - 1];
                ASSERT(extendedMetadata.record->opCount == 2, "Expected 2 operands for extended metadata");

                // Optional element type
                const Backend::IL::Type* elementType{nullptr};

                // Optional texel format
                Backend::IL::Format format{Backend::IL::Format::None};

                // Parse tags
                for (uint32_t kv = 0; kv < extendedMetadata.record->opCount; kv += 2) {
                    switch (static_cast<DXILUAVTag>(GetOperandU32Constant(resource.Op(kv + 0)))) {
                        case DXILUAVTag::ElementType: {
                            // Get type
                            auto componentType = GetOperandU32Constant<ComponentType>(extendedMetadata.record->Op(kv + 1));

                            // Get type and format
                            elementType = GetComponentType(componentType);
                            format = GetComponentFormat(componentType);
                            break;
                        }
                        case DXILUAVTag::ByteStride: {
                            break;
                        }
                    }
                }

                // Buffer shape?
                if (IsBuffer(shape)) {
                    Backend::IL::BufferType buffer{};
                    buffer.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    buffer.elementType = elementType;
                    buffer.texelType = format;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(buffer);
                } else {
                    Backend::IL::TextureType texture{};
                    texture.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    texture.sampledType = elementType;
                    texture.format = format;

                    // Translate shape
                    switch (shape) {
                        default:
                            texture.dimension = Backend::IL::TextureDimension::Unexposed;
                            break;
                        case DXILShaderResourceShape::Invalid:
                            break;
                        case DXILShaderResourceShape::Texture1D:
                            texture.dimension = Backend::IL::TextureDimension::Texture1D;
                            break;
                        case DXILShaderResourceShape::Texture2D:
                            texture.dimension = Backend::IL::TextureDimension::Texture2D;
                            break;
                        case DXILShaderResourceShape::Texture2DMS:
                            texture.dimension = Backend::IL::TextureDimension::Texture2D;
                            texture.multisampled = true;
                            break;
                        case DXILShaderResourceShape::Texture3D:
                            texture.dimension = Backend::IL::TextureDimension::Texture3D;
                            break;
                        case DXILShaderResourceShape::TextureCube:
                            texture.dimension = Backend::IL::TextureDimension::Texture2DCube;
                            break;
                        case DXILShaderResourceShape::Texture1DArray:
                            texture.dimension = Backend::IL::TextureDimension::Texture1DArray;
                            break;
                        case DXILShaderResourceShape::Texture2DArray:
                            texture.dimension = Backend::IL::TextureDimension::Texture2DArray;
                            break;
                        case DXILShaderResourceShape::Texture2DMSArray:
                            texture.dimension = Backend::IL::TextureDimension::Texture2DArray;
                            texture.multisampled = true;
                            break;
                        case DXILShaderResourceShape::TextureCubeArray:
                            texture.dimension = Backend::IL::TextureDimension::Texture2DCubeArray;
                            break;
                    }

                    entry.type = program.GetTypeMap().FindTypeOrAdd(texture);
                }
                break;
            }
            case DXILShaderResourceClass::CBVs: {
                // Unique ops
                uint64_t byteSize = GetOperandU32Constant(resource.Op(6));
                uint64_t extendedMetadata = GetOperandU32Constant(resource.Op(7));
                break;
            }
            case DXILShaderResourceClass::Samplers: {
                // Unique ops
                uint64_t samplerType = GetOperandU32Constant(resource.Op(6));
                uint64_t extendedMetadata = GetOperandU32Constant(resource.Op(7));
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

const Backend::IL::Type *DXILPhysicalBlockMetadata::GetComponentType(ComponentType type) {
    switch (type) {
        default:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType{});
        case ComponentType::Int16:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 16,
                .signedness = true
            });
        case ComponentType::UInt16:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 16,
                .signedness = false
            });
        case ComponentType::Int32:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 32,
                .signedness = true
            });
        case ComponentType::UInt32:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 32,
                .signedness = false
            });
        case ComponentType::Int64:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 64,
                .signedness = true
            });
        case ComponentType::UInt64:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{
                .bitWidth = 64,
                .signedness = false
            });
        case ComponentType::FP16:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{
                .bitWidth = 16,
            });
        case ComponentType::FP32:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{
                .bitWidth = 32,
            });
        case ComponentType::FP64:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{
                .bitWidth = 64,
            });
        case ComponentType::UNormFP16:
        case ComponentType::SNormFP32:
        case ComponentType::UNormFP32:
        case ComponentType::SNormFP64:
        case ComponentType::UNormFP64:
        case ComponentType::SNormFP16:
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{
                .bitWidth = 16,
            });
            break;
    }
}

Backend::IL::Format DXILPhysicalBlockMetadata::GetComponentFormat(ComponentType type) {
    switch (type) {
        default:
            return Backend::IL::Format::Unexposed;
        case ComponentType::None:
            return Backend::IL::Format::None;
        case ComponentType::Int16:
            return Backend::IL::Format::R16Int;
        case ComponentType::UInt16:
            return Backend::IL::Format::R16UInt;
        case ComponentType::Int32:
            return Backend::IL::Format::R32Int;
        case ComponentType::UInt32:
            return Backend::IL::Format::R32UInt;
        case ComponentType::Int64:
            return Backend::IL::Format::Unexposed;
        case ComponentType::UInt64:
            return Backend::IL::Format::Unexposed;
        case ComponentType::FP16:
            return Backend::IL::Format::R16Float;
        case ComponentType::FP32:
            return Backend::IL::Format::R32Float;
        case ComponentType::FP64:
            return Backend::IL::Format::Unexposed;
        case ComponentType::SNormFP16:
            return Backend::IL::Format::R16Snorm;
        case ComponentType::UNormFP16:
            return Backend::IL::Format::R16Unorm;
        case ComponentType::SNormFP32:
            return Backend::IL::Format::R32Snorm;
        case ComponentType::UNormFP32:
            return Backend::IL::Format::R32Unorm;
        case ComponentType::SNormFP64:
            return Backend::IL::Format::Unexposed;
        case ComponentType::UNormFP64:
            return Backend::IL::Format::Unexposed;
        case ComponentType::PackedS8x32:
            return Backend::IL::Format::Unexposed;
        case ComponentType::PackedU8x32:
            return Backend::IL::Format::Unexposed;
    }
}
