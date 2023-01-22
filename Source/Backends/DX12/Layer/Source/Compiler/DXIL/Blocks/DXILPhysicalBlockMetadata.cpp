#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockMetadata.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXJob.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordView.h>

// Backend
#include <Backend/IL/TypeSize.h>

// Common
#include <Common/Sink.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockMetadata::DXILPhysicalBlockMetadata(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table) :
    DXILPhysicalBlockSection(allocators, program, table) {

    // Reset resources
    for (uint32_t i = 0; i < static_cast<uint32_t>(DXILShaderResourceClass::Count); i++) {
        resources.lists[i] = 0u;
    }
}

void DXILPhysicalBlockMetadata::CopyTo(DXILPhysicalBlockMetadata &out) {
    out.resources = resources;
    out.entryPoint = entryPoint;
    out.metadataBlocks = metadataBlocks;
    out.registerClasses = registerClasses;
    out.registerSpaces = registerSpaces;
    out.registerSpaceBound = registerSpaceBound;
    out.programMetadata = programMetadata;
    out.shadingModel = shadingModel;
    out.handles = handles;
}

void DXILPhysicalBlockMetadata::ParseMetadata(const struct LLVMBlock *block) {
    MetadataBlock& metadataBlock = metadataBlocks.emplace_back();
    metadataBlock.uid = block->uid;

    // Empty out previous block data
    metadataBlock.metadata.resize(block->records.Size());

    // Current name
    LLVMRecordStringView recordName;

    // Visit records
    for (uint32_t i = 0; i < static_cast<uint32_t>(block->records.Size()); i++) {
        const LLVMRecord &record = block->records[i];

        // Get metadata
        Metadata &md = metadataBlock.metadata[i];
        md.source = i;

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
                ParseNamedNode(metadataBlock, block, record, recordName, i);
                break;
            }

                // Constant value
            case LLVMMetadataRecord::Value: {
                if (table.idMap.GetType(record.Op(1)) == DXILIDType::Constant) {
                    auto mapped = table.idMap.GetMappedCheckType(record.Op(1), DXILIDType::Constant);
                    md.value.type = table.type.typeMap.GetType(record.Op32(0));
                    md.value.constant = program.GetConstants().GetConstant(mapped);
                    ASSERT(md.value.type, "Expected type");
                    ASSERT(md.value.constant, "Expected constant");
                }
                break;
            }
        }
    }
}

void DXILPhysicalBlockMetadata::ParseNamedNode(MetadataBlock& metadataBlock, const struct LLVMBlock *block, const LLVMRecord &record, const LLVMRecordStringView &name, uint32_t index) {
    switch (name.GetHash()) {
        // Resource declaration record
        case CRC64("dx.rootSignature"): {
            if (name != "dx.rootSignature") {
                return;
            }

            ASSERT(false, "Metadata embedded root signatures not implemented");
            break;
        }

        // Resource declaration record
        case CRC64("dx.resources"): {
            if (name != "dx.resources") {
                return;
            }

            // Get list
            ASSERT(record.opCount == 1, "Expected a single value for dx.resources");
            const LLVMRecord &list = block->records[record.Op(0)];

            // Set ids
            resources.uid = block->uid;
            resources.source = record.Op32(0);

            // Check classes
            for (uint32_t i = 0; i < static_cast<uint32_t>(DXILShaderResourceClass::Count); i++) {
                if (resources.lists[i] = list.Op32(i); resources.lists[i] != 0) {
                    ParseResourceList(metadataBlock, block, static_cast<DXILShaderResourceClass>(i), resources.lists[i]);
                }
            }
            break;
        }

        // Program entrypoints
        case CRC64("dx.entryPoints"): {
            if (name != "dx.entryPoints") {
                return;
            }

            // Get list
            ASSERT(record.opCount == 1, "Expected a single value for dx.entryPoints");
            const LLVMRecord &list = block->records[record.Op(0)];

            // Set ids
            entryPoint.uid = block->uid;
            entryPoint.program = record.Op32(0);

            // Extended metadata kv pairs?
            if (list.Op(4)) {
                const LLVMRecord &kvRecord = block->records[list.ops[4] - 1];

                // Parse tags
                for (uint32_t kv = 0; kv < kvRecord.opCount; kv += 2) {
                    switch (GetOperandU32Constant<DXILProgramTag>(metadataBlock, kvRecord.Op32(kv + 0))) {
                        default: {
                            break;
                        }
                        case DXILProgramTag::ShaderFlags: {
                            // Get current flags
                            programMetadata.shaderFlags = DXILProgramShaderFlagSet(GetOperandU32Constant(metadataBlock, kvRecord.Op32(kv + 1)));
                            break;
                        }
                    }
                }
            }
            break;
        }

            // Program entrypoints
        case CRC64("dx.shaderModel"): {
            if (name != "dx.shaderModel") {
                return;
            }

            // Get list
            ASSERT(record.opCount == 1, "Expected a single value for dx.shaderModel");
            const LLVMRecord &list = block->records[record.Op(0)];

            // Get shading model
            LLVMRecordStringView shadingModelStr(block->records[list.Op(0) - 1], 0);

            // Translate
            if (shadingModelStr == "cs") {
                shadingModel._class = DXILShadingModelClass::CS;
            } else if (shadingModelStr == "vs") {
                shadingModel._class = DXILShadingModelClass::VS;
            } else if (shadingModelStr == "ps") {
                shadingModel._class = DXILShadingModelClass::PS;
            } else if (shadingModelStr == "gs") {
                shadingModel._class = DXILShadingModelClass::GS;
            } else if (shadingModelStr == "ds") {
                shadingModel._class = DXILShadingModelClass::DS;
            } else if (shadingModelStr == "hs") {
                shadingModel._class = DXILShadingModelClass::HS;
            } else if (shadingModelStr == "as") {
                shadingModel._class = DXILShadingModelClass::AS;
            } else if (shadingModelStr == "ms") {
                shadingModel._class = DXILShadingModelClass::MS;
            }
        }
    }
}

void DXILPhysicalBlockMetadata::ParseResourceList(struct MetadataBlock& metadataBlock, const struct LLVMBlock *block, DXILShaderResourceClass type, uint32_t id) {
    const LLVMRecord &record = block->records[id - 1];

    // Get the class
    MappedRegisterClass& registerClass = FindOrAddRegisterClass(type);

    // For each resource
    for (uint32_t i = 0; i < record.opCount; i++) {
        const LLVMRecord &resource = block->records[record.ops[i] - 1];

        // Get handle id
        uint64_t resourceID = GetOperandU32Constant(metadataBlock, resource.Op32(0));

        // Undef constant
        const IL::Constant *constantPointer = GetOperandConstant(metadataBlock, resource.Op32(1));

        // Get pointer
        auto* constantPointerType = constantPointer->type->As<Backend::IL::PointerType>();

        // Contained texel type
        const Backend::IL::Type* containedType{nullptr};

        // Handle contained
        switch (constantPointerType->pointee->kind) {
            default: {
                ASSERT(false, "Unexpected resource constant pointer type");
                break;
            }
            case Backend::IL::TypeKind::Struct: {
                auto* constantStruct = constantPointerType->pointee->As<Backend::IL::StructType>();

                // Get resource types
                ASSERT(constantStruct->memberTypes.size() >= 1, "Unexpected metadata constant size for resource node");
                containedType = constantStruct->memberTypes[0];
                break;
            }
            case Backend::IL::TypeKind::Array: {
                // Must be array of struct of
                auto* constantArray = constantPointerType->pointee->As<Backend::IL::ArrayType>();
                auto* constantStruct = constantArray->elementType->As<Backend::IL::StructType>();

                // Get resource types
                ASSERT(constantStruct->memberTypes.size() >= 1, "Unexpected metadata constant size for resource node");
                containedType = constantStruct->memberTypes[0];
                break;
            }
        }


        // TODO: How on earth are the names stored?
        // uint64_t name = GetOperandU32Constant(resource.Op(2))->value;

        // Resource binding
        uint32_t bindSpace = GetOperandU32Constant(metadataBlock, resource.Op32(3));
        uint32_t rsBase = GetOperandU32Constant(metadataBlock, resource.Op32(4));
        uint32_t rsRange = GetOperandU32Constant(metadataBlock, resource.Op32(5));

        // Get the space
        UserRegisterSpace& registerSpace = FindOrAddRegisterSpace(bindSpace);

        // Prepare entry
        HandleEntry entry;
        entry.record = &resource;
        entry.bindSpace = bindSpace;
        entry.registerBase = rsBase;
        entry.registerRange = rsRange;

        // Update bound
        registerSpace.registerBound = std::max<uint32_t>(registerSpace.registerBound, entry.registerBase + entry.registerRange);

        // Handle based on type
        switch (type) {
            default:
            ASSERT(false, "Invalid resource type");
                break;
            case DXILShaderResourceClass::SRVs: {
                // Unique ops
                auto shape = GetOperandU32Constant<DXILShaderResourceShape>(metadataBlock, resource.Op32(6));
                uint64_t sampleCount = GetOperandU32Constant(metadataBlock, resource.Op32(7));

                // Unused
                GRS_SINK(sampleCount);

                // Optional element type
                const Backend::IL::Type* elementType{nullptr};

                // Optional texel format
                Backend::IL::Format format{Backend::IL::Format::None};

                // Has extended metadata?
                if (uint32_t extendedMetadataId = resource.Op32(8)) {
                    // Get extended metadata
                    Metadata &extendedMetadata = metadataBlock.metadata[extendedMetadataId - 1];

                    // Get extended record
                    const LLVMRecord& extendedRecord = block->records[extendedMetadata.source];
                    ASSERT(extendedRecord.opCount == 2, "Expected 2 operands for extended metadata");

                    // Parse tags
                    for (uint32_t kv = 0; kv < extendedRecord.opCount; kv += 2) {
                        switch (static_cast<DXILSRVTag>(GetOperandU32Constant(metadataBlock, resource.Op32(kv + 0)))) {
                            case DXILSRVTag::ElementType: {
                                // Get type
                                auto componentType = GetOperandU32Constant<ComponentType>(metadataBlock, extendedRecord.Op32(kv + 1));

                                // Get type and format
                                elementType = GetComponentType(componentType);
                                format = GetComponentFormat(componentType);
                                break;
                            }
                            case DXILSRVTag::ByteStride: {
                                break;
                            }
                        }
                    }
                }

                // Buffer shape?
                if (IsBuffer(shape)) {
                    Backend::IL::BufferType buffer{};
                    buffer.samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                    buffer.elementType = containedType;
                    buffer.texelType = format;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(buffer);
                } else {
                    Backend::IL::TextureType texture{};
                    texture.samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                    texture.sampledType = containedType;
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
            case DXILShaderResourceClass::UAVs: {
                // Unique ops
                auto shape = static_cast<DXILShaderResourceShape>(GetOperandU32Constant(metadataBlock, resource.Op32(6)));
                uint64_t globallyCoherent = GetOperandBoolConstant(metadataBlock, resource.Op32(7));
                uint64_t counter = GetOperandBoolConstant(metadataBlock, resource.Op32(8));
                uint64_t rasterizerOrderedView = GetOperandBoolConstant(metadataBlock, resource.Op32(9));

                // Unused
                GRS_SINK(globallyCoherent);
                GRS_SINK(counter);
                GRS_SINK(rasterizerOrderedView);

                // Optional element type
                const Backend::IL::Type *elementType{nullptr};

                // Optional texel format
                Backend::IL::Format format{Backend::IL::Format::None};

                // Has extended metadata?
                if (uint32_t extendedMetadataId = resource.Op32(10)) {
                    // Get extended metadata
                    Metadata &extendedMetadata = metadataBlock.metadata[extendedMetadataId - 1];

                    // Get extended record
                    const LLVMRecord &extendedRecord = block->records[extendedMetadata.source];
                    ASSERT(extendedRecord.opCount == 2, "Expected 2 operands for extended metadata");

                    // Parse tags
                    for (uint32_t kv = 0; kv < extendedRecord.opCount; kv += 2) {
                        switch (static_cast<DXILUAVTag>(GetOperandU32Constant(metadataBlock, resource.Op32(kv + 0)))) {
                            case DXILUAVTag::ElementType: {
                                // Get type
                                auto componentType = GetOperandU32Constant<ComponentType>(metadataBlock, extendedRecord.Op32(kv + 1));

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
                }

                // Buffer shape?
                if (IsBuffer(shape)) {
                    Backend::IL::BufferType buffer{};
                    buffer.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    buffer.elementType = containedType;
                    buffer.texelType = format;
                    entry.type = program.GetTypeMap().FindTypeOrAdd(buffer);
                } else {
                    Backend::IL::TextureType texture{};
                    texture.samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                    texture.sampledType = containedType;
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
                uint64_t byteSize = GetOperandU32Constant(metadataBlock, resource.Op32(6));

                // Unused
                GRS_SINK(byteSize);

                if (uint32_t nullableMetadata = resource.Op32(7)) {
                    uint64_t extendedMetadata = GetOperandU32Constant(metadataBlock, nullableMetadata);

                    // Unused
                    GRS_SINK(extendedMetadata);
                }

                Backend::IL::CBufferType cbuffer{};
                entry.type = program.GetTypeMap().FindTypeOrAdd(cbuffer);
                break;
            }
            case DXILShaderResourceClass::Samplers: {
                // Unique ops
                uint64_t samplerType = GetOperandU32Constant(metadataBlock, resource.Op32(6));

                // Unused
                GRS_SINK(samplerType);

                if (uint32_t nullableMetadata = resource.Op32(7)) {
                    uint64_t extendedMetadata = GetOperandU32Constant(metadataBlock, nullableMetadata);

                    // Unused
                    GRS_SINK(extendedMetadata);
                }

                Backend::IL::SamplerType sampler{};
                entry.type = program.GetTypeMap().FindTypeOrAdd(sampler);
                break;
            }
        }

        // Push handle
        uint32_t handleID = static_cast<uint32_t>(handles.size());
        handles.push_back(entry);

        // Ensure space
        if (registerClass.resourceLookup.size() <= resourceID) {
            registerClass.resourceLookup.resize(resourceID + 1);
        }

        // Set entry at id
        registerClass.resourceLookup[resourceID] = handleID;

        // Add handles
        registerClass.handles.push_back(handleID);
        registerSpace.handles.push_back(handleID);
    }
}

const Backend::IL::Type *DXILPhysicalBlockMetadata::GetHandleType(DXILShaderResourceClass _class, uint32_t handleID) {
    const HandleEntry* handle = GetHandle(_class, handleID);
    if (!handle) {
        return nullptr;
    }

    return handle->type;
}

const DXILPhysicalBlockMetadata::HandleEntry* DXILPhysicalBlockMetadata::GetHandle(DXILShaderResourceClass _class, uint32_t handleID) {
    MappedRegisterClass& registerClass = FindOrAddRegisterClass(_class);

    if (registerClass.resourceLookup.size() <= handleID) {
        return nullptr;
    }

    HandleEntry& handle = handles[registerClass.resourceLookup[handleID]];
    if (!handle.record) {
        return nullptr;
    }

    // Get entry
    return &handle;
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

ComponentType DXILPhysicalBlockMetadata::GetFormatComponent(Backend::IL::Format format) {
    switch (format) {
        default:
            ASSERT(false, "Invalid value");
            return ComponentType::None;
        case Backend::IL::Format::RGBA32Float:
            return ComponentType::FP32;
        case Backend::IL::Format::RGBA16Float:
            return ComponentType::FP16;
        case Backend::IL::Format::R32Float:
            return ComponentType::FP32;
        case Backend::IL::Format::R32Snorm:
            return ComponentType::SNormFP32;
        case Backend::IL::Format::R32Unorm:
            return ComponentType::UNormFP32;
        case Backend::IL::Format::RGBA8:
            return ComponentType::FP32;
        case Backend::IL::Format::RGBA8Snorm:
            return ComponentType::SNormFP32;
        case Backend::IL::Format::RG32Float:
            return ComponentType::FP32;
        case Backend::IL::Format::RG16Float:
            return ComponentType::FP16;
        case Backend::IL::Format::R11G11B10Float:
            return ComponentType::FP32;
        case Backend::IL::Format::R16Float:
            return ComponentType::FP16;
        case Backend::IL::Format::RGBA16:
            return ComponentType::FP16;
        case Backend::IL::Format::RGB10A2:
            return ComponentType::FP32;
        case Backend::IL::Format::RG16:
            return ComponentType::FP16;
        case Backend::IL::Format::RG8:
            return ComponentType::FP32;
        case Backend::IL::Format::R16:
            return ComponentType::FP16;
        case Backend::IL::Format::R8:
            return ComponentType::FP32;
        case Backend::IL::Format::RGBA16Snorm:
            return ComponentType::SNormFP16;
        case Backend::IL::Format::RG16Snorm:
            return ComponentType::SNormFP16;
        case Backend::IL::Format::RG8Snorm:
            return ComponentType::SNormFP32;
        case Backend::IL::Format::R16Snorm:
            return ComponentType::SNormFP16;
        case Backend::IL::Format::R16Unorm:
            return ComponentType::UNormFP16;
        case Backend::IL::Format::R8Snorm:
            return ComponentType::SNormFP16;
        case Backend::IL::Format::RGBA32Int:
            return ComponentType::Int32;
        case Backend::IL::Format::RGBA16Int:
            return ComponentType::Int16;
        case Backend::IL::Format::RGBA8Int:
            return ComponentType::Int32;
        case Backend::IL::Format::R32Int:
            return ComponentType::Int32;
        case Backend::IL::Format::RG32Int:
            return ComponentType::Int32;
        case Backend::IL::Format::RG16Int:
            return ComponentType::Int16;
        case Backend::IL::Format::RG8Int:
            return ComponentType::Int32;
        case Backend::IL::Format::R16Int:
            return ComponentType::Int16;
        case Backend::IL::Format::R8Int:
            return ComponentType::Int32;
        case Backend::IL::Format::RGBA32UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::RGBA16UInt:
            return ComponentType::UInt16;
        case Backend::IL::Format::RGBA8UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::R32UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::RGB10a2UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::RG32UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::RG16UInt:
            return ComponentType::UInt16;
        case Backend::IL::Format::RG8UInt:
            return ComponentType::UInt32;
        case Backend::IL::Format::R16UInt:
            return ComponentType::UInt16;
        case Backend::IL::Format::R8UInt:
            return ComponentType::UInt32;
    }
}

void DXILPhysicalBlockMetadata::CompileMetadata(struct LLVMBlock *block) {
}

void DXILPhysicalBlockMetadata::CompileMetadata(const DXJob& job) {
    // Compile all flags
    CompileProgramFlags(job);

    // Ensure capabilities
    EnsureUAVCapability();
    
    // Compile entry point definitions
    CompileProgramEntryPoints();

    // Ensure the program has a class list record
    EnsureProgramResourceClassList(job);

    // Create all resource classes
    CompileSRVResourceClass(job);
    CompileUAVResourceClass(job);
    CompileCBVResourceClass(job);
}

void DXILPhysicalBlockMetadata::StitchMetadata(struct LLVMBlock *block) {
    // Set source result for stitching
    for (uint32_t i = 0; i < static_cast<uint32_t>(block->records.Size()); i++) {
        LLVMRecord &record = block->records[i];
        record.result = i;
    }

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(block->uid);
    
    // Source to stitched mappings
    metadataBlock->sourceMappings.resize(block->records.Size(), ~0u);

    // Swap source data
    TrivialStackVector<LLVMRecord, 32> source;
    block->records.Swap(source);

    // Swap element data
    TrivialStackVector<LLVMBlockElement, 128> elements;
    block->elements.Swap(elements);

    // Reserve
    block->elements.Reserve(elements.Size());

    // Filter all records
    for (const LLVMBlockElement &element: elements) {
        if (!element.Is(LLVMBlockElementType::Record)) {
            block->elements.Add(element);
        }
    }

    // Resolver loop
    for (;;) {
        bool mutated = false;

        for (size_t i = 0; i < source.Size(); i++) {
            LLVMRecord &record = source[i];

            if (record.result == ~0u) {
                continue;
            }

            // Handle record
            switch (static_cast<LLVMMetadataRecord>(record.id)) {
                default: {
                    // Set new mapping
                    metadataBlock->sourceMappings.at(record.result) = block->records.Size();

                    // Append
                    block->AddRecord(record);
                    record.result = ~0u;

                    // OK
                    mutated = true;
                    break;
                }

                case LLVMMetadataRecord::Name: {
                    // Ignore named blocks
                    i++;
                    break;
                }

                case LLVMMetadataRecord::Node:
                case LLVMMetadataRecord::DistinctNode: {
                    bool resolved = true;

                    // Ensure all operands are resolved
                    for (uint32_t opId = 0; opId < record.opCount; opId++) {
                        // Distinct nodes may refer to themselves
                        if (record.ops[opId] != 0 && record.ops[opId] - 1 != i) {
                            resolved &= metadataBlock->sourceMappings.at(record.ops[opId] - 1) != ~0u;
                        }
                    }

                    // Ready?
                    if (resolved) {
                        // Set new mapping
                        metadataBlock->sourceMappings.at(record.result) = block->records.Size();

                        // Stitch ops
                        for (uint32_t opId = 0; opId < record.opCount; opId++) {
                            if (record.ops[opId] != 0) {
                                record.ops[opId] = metadataBlock->sourceMappings.at(record.ops[opId] - 1) + 1;
                            }
                        }

                        // Append
                        block->AddRecord(record);
                        record.result = ~0u;

                        // OK
                        mutated = true;
                    }
                    break;
                }

                    // Constant value
                case LLVMMetadataRecord::Value: {
                    // Stitch the value id
                    table.idRemapper.Remap(record.Op(1));

                    // Set new mapping
                    metadataBlock->sourceMappings.at(record.result) = block->records.Size();

                    // Append
                    block->AddRecord(record);
                    record.result = ~0u;

                    // OK
                    mutated = true;
                    break;
                }
            }
        }

        // No changes since last iteration?
        if (!mutated) {
            break;
        }
    }

    // Append named blocks
    for (size_t i = 0; i < source.Size(); i++) {
        // Name?
        if (!source[i].Is(LLVMMetadataRecord::Name)) {
            continue;
        }

        LLVMRecord &name = source[i];
        LLVMRecord &node = source[i + 1];

        // Stitch ops
        for (uint32_t opId = 0; opId < node.opCount; opId++) {
            if (node.ops[opId] != 0) {
                node.ops[opId] = metadataBlock->sourceMappings.at(node.ops[opId]);
            }
        }

        // Mark
        name.result = ~0u;
        node.result = ~0u;

        // Append
        block->AddRecord(name);
        block->AddRecord(node);
    }

    // Must have resolved all
    if (block->records.Size() != source.Size()) {
        ASSERT(false, "Failed to resolve metadata");
    }
}

void DXILPhysicalBlockMetadata::SetDeclarationBlock(struct LLVMBlock *block) {
    declarationBlock = block;
}

uint32_t DXILPhysicalBlockMetadata::FindOrAddString(DXILPhysicalBlockMetadata::MetadataBlock &metadata, LLVMBlock *block, const std::string_view& str) {
    // Check if exists
    for (uint32_t i = 0; i < metadata.metadata.size(); i++) {
        if (metadata.metadata[i].name == str) {
            return i + 1;
        }
    }

    // Add value md
    Metadata& md = metadata.metadata.emplace_back();
    md.source = static_cast<uint32_t>(block->records.Size());
    md.name = str;

    // Insert value record
    LLVMRecord record(LLVMMetadataRecord::StringOld);
    record.opCount = static_cast<uint32_t>(str.length());
    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);

    // Copy string
    for (size_t i = 0; i < str.length(); i++) {
        record.ops[i] = static_cast<uint64_t>(str[i]);
    }

    block->AddRecord(record);

    // OK
    return static_cast<uint32_t>(metadata.metadata.size());
}

uint32_t DXILPhysicalBlockMetadata::FindOrAddOperandConstant(DXILPhysicalBlockMetadata::MetadataBlock &metadata, LLVMBlock *block, const Backend::IL::Constant *constant) {
    // Check if exists
    for (uint32_t i = 0; i < metadata.metadata.size(); i++) {
        if (metadata.metadata[i].value.constant == constant) {
            return i + 1;
        }
    }

    // Add value md
    Metadata& md = metadata.metadata.emplace_back();
    md.source = static_cast<uint32_t>(block->records.Size());
    md.value.type = constant->type;
    md.value.constant = constant;

    // Insert value record
    LLVMRecord record(LLVMMetadataRecord::Value);
    record.opCount = 2;
    record.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
    record.ops[0] = table.type.typeMap.GetType(constant->type);
    record.ops[1] = DXILIDRemapper::EncodeUserOperand(constant->id);
    block->AddRecord(record);

    // OK
    return static_cast<uint32_t>(metadata.metadata.size());
}

void DXILPhysicalBlockMetadata::EnsureUAVCapability() {
    // CS, PS is implicit
    if (shadingModel._class == DXILShadingModelClass::CS || shadingModel._class == DXILShadingModelClass::PS) {
        return;
    }

    // Either or already implies UAV capabilities
    if (programMetadata.shaderFlags & (DXILProgramShaderFlag::UseRelaxedTypedUAVLoads | DXILProgramShaderFlag::UseUAVs | DXILProgramShaderFlag::Use64UAVs)) {
        return;
    }

    // Enable the base UAV mask
    programMetadata.internalShaderFlags |= DXILProgramShaderFlag::UseUAVs;
}

void DXILPhysicalBlockMetadata::AddProgramFlag(DXILProgramShaderFlagSet flags) {
    programMetadata.internalShaderFlags |= flags;
}

void DXILPhysicalBlockMetadata::EnsureProgramResourceClassList(const DXJob &job) {
    // Program may already have a list
    if (resources.uid != ~0u) {
        return;
    }

    // Get metadata block
    LLVMBlock* mdBlock = table.scan.GetRoot().GetBlock(LLVMReservedBlock::Metadata);

    // Set block
    resources.uid = mdBlock->uid;

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(resources.uid);

    // Class identifier (+1 for nullability)
    Metadata& classMd = metadataBlock->metadata.emplace_back();
    classMd.source = static_cast<uint32_t>(mdBlock->records.Size());
    uint32_t classId = classMd.source + 1;

    // Class list, points to the individual class handles
    LLVMRecord classRecord(LLVMMetadataRecord::Node);
    classRecord.opCount = static_cast<uint32_t>(DXILShaderResourceClass::Count);
    classRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(classRecord.opCount);
    classRecord.ops[static_cast<uint32_t>(DXILShaderResourceClass::CBVs)] = 0;
    classRecord.ops[static_cast<uint32_t>(DXILShaderResourceClass::SRVs)] = 0;
    classRecord.ops[static_cast<uint32_t>(DXILShaderResourceClass::UAVs)] = 0;
    classRecord.ops[static_cast<uint32_t>(DXILShaderResourceClass::Samplers)] = 0;
    mdBlock->AddRecord(classRecord);

    // Set source for later lookup
    resources.source = static_cast<uint32_t>(mdBlock->records.Size() - 1);

    // Get the program block
    LLVMRecord& programRecord = declarationBlock->GetBlockWithUID(entryPoint.uid)->records[entryPoint.program];

    // Set class id at program
    ASSERT(programRecord.Op(3) == 0, "Program record already a resource class node");
    programRecord.Op(3) = classId;

    // Expected name
    const char* name = "dx.resources";

    // Add md
    metadataBlock->metadata.emplace_back().source = static_cast<uint32_t>(mdBlock->records.Size());

    // Name dx.resources
    LLVMRecord nameRecord(LLVMMetadataRecord::Name);
    nameRecord.opCount = static_cast<uint32_t>(std::strlen(name));
    nameRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(nameRecord.opCount);
    std::copy(name, name + nameRecord.opCount, nameRecord.ops);
    mdBlock->AddRecord(nameRecord);

    // Add md
    metadataBlock->metadata.emplace_back().source = static_cast<uint32_t>(mdBlock->records.Size());

    // Insert dx.resources, points to the class list
    LLVMRecord dxResourceRecord(LLVMMetadataRecord::NamedNode);
    dxResourceRecord.opCount = 1u;
    dxResourceRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(1u);
    dxResourceRecord.ops[0] = classId - 1;
    mdBlock->AddRecord(dxResourceRecord);
}

void DXILPhysicalBlockMetadata::CreateResourceHandles(const DXJob& job) {
    CreateShaderExportHandle(job);
    CreatePRMTHandle(job);
    CreateDescriptorHandle(job);
    CreateEventHandle(job);
    CreateShaderDataHandles(job);
}

void DXILPhysicalBlockMetadata::CreateShaderExportHandle(const DXJob& job) {
    // i32
    const Backend::IL::Type* i32 = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32,.signedness=true});

    // {i32}
    const Backend::IL::Type* retTy = program.GetTypeMap().FindTypeOrAdd(Backend::IL::StructType {
        .memberTypes = { i32 }
    });

    // {i32}[count]
    const Backend::IL::Type* retArrayTy = program.GetTypeMap().FindTypeOrAdd(Backend::IL::ArrayType{
        .elementType = retTy,
        .count = job.instrumentationKey.bindingInfo.shaderExportCount
    });

    // Compile as named
    table.type.typeMap.CompileNamedType(retTy, "class.RWBuffer<unsigned int>");

    // {i32}*
    const Backend::IL::Type* retTyPtr = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = retArrayTy,
        .addressSpace = Backend::IL::AddressSpace::Function
    });

    // Create handle
    HandleEntry& handle = handles.emplace_back();
    handle.name = "ShaderExport";
    handle.type = retTyPtr;
    handle.bindSpace = job.instrumentationKey.bindingInfo.space;
    handle.registerBase = job.instrumentationKey.bindingInfo.shaderExportBaseRegister;
    handle.registerRange = job.instrumentationKey.bindingInfo.shaderExportCount;
    handle.uav.componentType = ComponentType::UInt32;
    handle.uav.shape = DXILShaderResourceShape::TypedBuffer;

    // Append handle to class
    MappedRegisterClass& _class = FindOrAddRegisterClass(DXILShaderResourceClass::UAVs);
    _class.handles.push_back(static_cast<uint32_t>(handles.size()) - 1);

    // Set binding info
    table.bindingInfo.shaderExportHandleId = static_cast<uint32_t>(_class.handles.size()) - 1;
}

void DXILPhysicalBlockMetadata::CreatePRMTHandle(const DXJob &job) {
    // i32
    const Backend::IL::Type* i32 = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32,.signedness=true});

    // {i32}
    const Backend::IL::Type* retTy = program.GetTypeMap().FindTypeOrAdd(Backend::IL::StructType {
        .memberTypes = { i32 }
    });

    // Compile as named
    table.type.typeMap.CompileNamedType(retTy, "class.Buffer<unsigned int>");

    // {i32}*
    const Backend::IL::Type* retTyPtr = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = retTy,
        .addressSpace = Backend::IL::AddressSpace::Function
    });

    // Create handle
    HandleEntry& handle = handles.emplace_back();
    handle.name = "PRMT";
    handle.type = retTyPtr;
    handle.bindSpace = job.instrumentationKey.bindingInfo.space;
    handle.registerBase = job.instrumentationKey.bindingInfo.prmtBaseRegister;
    handle.registerRange = 1u;
    handle.srv.componentType = ComponentType::UInt32;
    handle.srv.shape = DXILShaderResourceShape::TypedBuffer;

    // Append handle to class
    MappedRegisterClass& _class = FindOrAddRegisterClass(DXILShaderResourceClass::SRVs);
    _class.handles.push_back(static_cast<uint32_t>(handles.size()) - 1);

    // Set binding info
    table.bindingInfo.prmtHandleId = static_cast<uint32_t>(_class.handles.size()) - 1;
}

void DXILPhysicalBlockMetadata::CreateDescriptorHandle(const DXJob &job) {
    // i32
    const Backend::IL::Type *i32 = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true});
    const Backend::IL::Type *i32x4 = program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType{.containedType=i32, .dimension=4});

    // {[i32 x 4]}
    const Backend::IL::Type* cbufferType = program.GetTypeMap().FindTypeOrAdd(Backend::IL::StructType {
        .memberTypes = {
            // [i32 x 4]
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::ArrayType {
                .elementType = i32x4,
                .count = MaxRootSignatureDWord / 4u
            })
        }
    });

    // Compile as named
    table.type.typeMap.CompileNamedType(cbufferType, "CBufferDescriptorData");

    // {[i32 x 4]}*
    const Backend::IL::Type* cbufferTypePtr = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = cbufferType,
        .addressSpace = Backend::IL::AddressSpace::Function
    });

    // Create handle
    HandleEntry& handle = handles.emplace_back();
    handle.name = "CBufferDescriptorData";
    handle.type = cbufferTypePtr;
    handle.bindSpace = job.instrumentationKey.bindingInfo.space;
    handle.registerBase = job.instrumentationKey.bindingInfo.descriptorConstantBaseRegister;
    handle.registerRange = 1u;

    // Append handle to class
    MappedRegisterClass& _class = FindOrAddRegisterClass(DXILShaderResourceClass::CBVs);
    _class.handles.push_back(static_cast<uint32_t>(handles.size()) - 1);

    // Set binding info
    table.bindingInfo.descriptorConstantsHandleId = static_cast<uint32_t>(_class.handles.size()) - 1;
}

void DXILPhysicalBlockMetadata::CreateEventHandle(const DXJob &job) {
    IL::ShaderDataMap& shaderDataMap = table.program.GetShaderDataMap();

    // Requested dword count
    uint32_t dwordCount = 0;

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type == ShaderDataType::Event) {
            dwordCount++;
        }
    }

    // i32
    const Backend::IL::Type* i32 = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32,.signedness=false});

    // Determine number of dwords
    uint32_t alignedDWords = dwordCount / 4;

    // Emit aligned data
    Backend::IL::StructType eventStruct;
    for (uint32_t i = 0; i < alignedDWords; i++) {
        eventStruct.memberTypes.push_back(program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType {
            .containedType = i32,
            .dimension = 4
        }));
    }

    // Number of unaligned dwords
    uint32_t unalignedDWords = dwordCount % 4;

    // Emit final unaligned (to the end offset) data
    if (unalignedDWords) {
        // Naked single?
        if (unalignedDWords == 1) {
            eventStruct.memberTypes.push_back(i32);
        } else {
            eventStruct.memberTypes.push_back(program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType {
                .containedType = i32,
                .dimension = static_cast<uint8_t>(unalignedDWords)
            }));
        }
    }

    // {[4xN] N-1}
    const Backend::IL::Type* cbufferType = program.GetTypeMap().FindTypeOrAdd(eventStruct);

    // Compile as named
    table.type.typeMap.CompileNamedType(cbufferType, "CBufferEventData");

    // {...}*
    const Backend::IL::Type* cbufferTypePtr = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = cbufferType,
        .addressSpace = Backend::IL::AddressSpace::Function
    });

    // Create handle
    HandleEntry& handle = handles.emplace_back();
    handle.name = "CBufferEventData";
    handle.type = cbufferTypePtr;
    handle.bindSpace = job.instrumentationKey.bindingInfo.space;
    handle.registerBase = job.instrumentationKey.bindingInfo.eventConstantBaseRegister;
    handle.registerRange = 1u;

    // Append handle to class
    MappedRegisterClass& _class = FindOrAddRegisterClass(DXILShaderResourceClass::CBVs);
    _class.handles.push_back(static_cast<uint32_t>(handles.size()) - 1);

    // Set binding info
    table.bindingInfo.eventConstantsHandleId = static_cast<uint32_t>(_class.handles.size()) - 1;
}

void DXILPhysicalBlockMetadata::CreateShaderDataHandles(const DXJob& job) {
    IL::ShaderDataMap& shaderDataMap = table.program.GetShaderDataMap();

    // All shader Datas are UAVs
    MappedRegisterClass& _class = FindOrAddRegisterClass(DXILShaderResourceClass::UAVs);

    // Set binding info
    // Handles are allocated linearly after the current index
    table.bindingInfo.shaderDataHandleId = static_cast<uint32_t>(_class.handles.size());

    // Current register offset
    uint32_t registerOffset{0};

    for (const ShaderDataInfo& info : shaderDataMap) {
        if (!(info.type & ShaderDataType::DescriptorMask)) {
            continue;
        }

        // Only buffers supported for now
        ASSERT(info.type == ShaderDataType::Buffer, "Only buffers are implemented for now");

        // Get mapped id
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);
        ASSERT(variable, "Failed to match variable to shader Data");

        // Variables always pointer to
        const auto* pointerType = variable->type->As<Backend::IL::PointerType>();

        // {format}
        const Backend::IL::Type* retTy = program.GetTypeMap().FindTypeOrAdd(Backend::IL::StructType {
            .memberTypes = { pointerType->pointee->As<Backend::IL::BufferType>()->elementType }
        });

        // Compile as named
        table.type.typeMap.CompileNamedType(retTy, "class.Buffer<Format>");

        // {format}*
        const Backend::IL::Type* retTyPtr = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType{
            .pointee = retTy,
            .addressSpace = Backend::IL::AddressSpace::Function
        });

        // Create handle
        HandleEntry& handle = handles.emplace_back();
        handle.name = "ShaderResource";
        handle.type = retTyPtr;
        handle.bindSpace = job.instrumentationKey.bindingInfo.space;
        handle.registerBase = job.instrumentationKey.bindingInfo.shaderResourceBaseRegister + registerOffset;
        handle.registerRange = 1u;
        handle.uav.componentType = GetFormatComponent(info.buffer.format);
        handle.uav.shape = DXILShaderResourceShape::TypedBuffer;

        // Append handle to class
        _class.handles.push_back(static_cast<uint32_t>(handles.size()) - 1);

        // Next
        registerOffset++;
    }
}

LLVMRecordView DXILPhysicalBlockMetadata::CompileResourceClassRecord(const MappedRegisterClass& mapped) {
    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(resources.uid);

    // Get the block
    LLVMBlock* block = declarationBlock->GetBlockWithUID(resources.uid);

    // Final record
    LLVMRecordView classRecord;

    // Existing record?
    if (resources.lists[static_cast<uint32_t>(mapped._class)] != 0u) {
        // Get existing record
        classRecord = LLVMRecordView(block, resources.lists[static_cast<uint32_t>(mapped._class)] - 1);

        // Extend operands
        auto* ops = table.recordAllocator.AllocateArray<uint64_t>(static_cast<uint32_t>(mapped.handles.size()));
        std::memcpy(ops, classRecord->ops, sizeof(uint64_t) * classRecord->opCount);

        // Set operands
        classRecord->opCount = static_cast<uint32_t>(mapped.handles.size());
        classRecord->ops = ops;
    } else {
        // Emplace record
        block->records.Add();

        // Allocate new list record
        classRecord = LLVMRecordView(block, static_cast<uint32_t>(block->records.Size()) - 1);
        classRecord->id = static_cast<uint32_t>(LLVMMetadataRecord::Node);
        classRecord->ops = table.recordAllocator.AllocateArray<uint64_t>(static_cast<uint32_t>(mapped.handles.size()));
        classRecord->opCount = static_cast<uint32_t>(mapped.handles.size());

        // Set new UAV node, +1 for nullability
        resources.lists[static_cast<uint32_t>(mapped._class)] = static_cast<uint32_t>(block->records.Size());

        // Create resource metadata row
        Metadata& uavMd = metadataBlock->metadata.emplace_back();
        uavMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Get class record
        LLVMRecord& classListRecord = block->records[resources.source];
        ASSERT(classListRecord.opCount == 4, "Invalid class record");

        // Redirect UAV
        classListRecord.Op(static_cast<uint32_t>(mapped._class)) = resources.lists[static_cast<uint32_t>(mapped._class)];
    }

    return classRecord;
}

void DXILPhysicalBlockMetadata::CompileSRVResourceClass(const DXJob &job) {
    // Get mapped
    MappedRegisterClass& mapped = FindOrAddRegisterClass(DXILShaderResourceClass::SRVs);

    // None to emit?
    if (mapped.handles.empty()) {
        return;
    }

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(resources.uid);

    // Get the block
    LLVMBlock* block = declarationBlock->GetBlockWithUID(resources.uid);

    // Compile the record
    LLVMRecordView classRecord = CompileResourceClassRecord(mapped);

    // Populate handles
    for (uint32_t i = 0; i < static_cast<uint32_t>(mapped.handles.size()); i++) {
        const HandleEntry& handle = handles[mapped.handles[i]];

        // Parsed handle?
        if (handle.record) {
            continue;
        }

        // Insert extended record node
        LLVMRecord extendedMetadata(LLVMMetadataRecord::Node);
        extendedMetadata.SetUser(false, ~0u, ~0u);
        extendedMetadata.opCount = 2;
        extendedMetadata.ops = table.recordAllocator.AllocateArray<uint64_t>(extendedMetadata.opCount);
        extendedMetadata.ops[0] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(DXILSRVTag::ElementType));
        extendedMetadata.ops[1] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(handle.srv.componentType));
        block->AddRecord(extendedMetadata);

        // Create extended metadata row
        Metadata& extendedMd = metadataBlock->metadata.emplace_back();
        extendedMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Index of extended node
        uint32_t extendedMdIndex = static_cast<uint32_t>(metadataBlock->metadata.size());

        // Insert resource record node
        LLVMRecord resource(LLVMMetadataRecord::Node);
        resource.SetUser(false, ~0u, ~0u);
        resource.opCount = 9;
        resource.ops = table.recordAllocator.AllocateArray<uint64_t>(resource.opCount);
        resource.ops[0] = FindOrAddOperandU32Constant(*metadataBlock, block, i);
        resource.ops[1] = FindOrAddOperandConstant(*metadataBlock, block, program.GetConstants().FindConstantOrAdd(handle.type, Backend::IL::UndefConstant{}));
        resource.ops[2] = FindOrAddString(*metadataBlock, block, handle.name);
        resource.ops[3] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.bindSpace);
        resource.ops[4] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerBase);
        resource.ops[5] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerRange);
        resource.ops[6] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(handle.srv.shape));
        resource.ops[7] = FindOrAddOperandU32Constant(*metadataBlock, block, 0u);
        resource.ops[8] = extendedMdIndex;
        block->AddRecord(resource);

        // Create resource metadata row
        Metadata& resourceMd = metadataBlock->metadata.emplace_back();
        resourceMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Insert handle
        classRecord->ops[i] = metadataBlock->metadata.size();
    }
}


void DXILPhysicalBlockMetadata::CompileProgramFlags(const DXJob &job) {
    // Get mapped
    MappedRegisterClass& mapped = FindOrAddRegisterClass(DXILShaderResourceClass::UAVs);

    // Total number of UAVs
    uint32_t count = 0;

    // Accumulate all handles
    for (size_t i = 0; i < mapped.handles.size(); i++) {
        const HandleEntry &handle = handles[mapped.handles[i]];
        count += handle.registerRange;
    }

    // Exceeded 8?
    if (count > 8) {
        AddProgramFlag(DXILProgramShaderFlag::Use64UAVs);
    }
}

void DXILPhysicalBlockMetadata::CompileUAVResourceClass(const DXJob &job) {
    // Get mapped
    MappedRegisterClass& mapped = FindOrAddRegisterClass(DXILShaderResourceClass::UAVs);

    // None to emit?
    if (mapped.handles.empty()) {
        return;
    }

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(resources.uid);

    // Get the block
    LLVMBlock* block = declarationBlock->GetBlockWithUID(resources.uid);

    // Compile the record
    LLVMRecordView classRecord = CompileResourceClassRecord(mapped);

    // Populate handles
    for (uint32_t i = 0; i < static_cast<uint32_t>(mapped.handles.size()); i++) {
        const HandleEntry& handle = handles[mapped.handles[i]];

        // Parsed handle?
        if (handle.record) {
            continue;
        }

        // Insert extended record node
        LLVMRecord extendedMetadata(LLVMMetadataRecord::Node);
        extendedMetadata.SetUser(false, ~0u, ~0u);
        extendedMetadata.opCount = 2;
        extendedMetadata.ops = table.recordAllocator.AllocateArray<uint64_t>(extendedMetadata.opCount);
        extendedMetadata.ops[0] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(DXILUAVTag::ElementType));
        extendedMetadata.ops[1] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(handle.uav.componentType));
        block->AddRecord(extendedMetadata);

        // Create extended metadata row
        Metadata& extendedMd = metadataBlock->metadata.emplace_back();
        extendedMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Index of extended node
        uint32_t extendedMdIndex = static_cast<uint32_t>(metadataBlock->metadata.size());

        // Insert resource record node
        LLVMRecord resource(LLVMMetadataRecord::Node);
        resource.SetUser(false, ~0u, ~0u);
        resource.opCount = 11;
        resource.ops = table.recordAllocator.AllocateArray<uint64_t>(resource.opCount);
        resource.ops[0] = FindOrAddOperandU32Constant(*metadataBlock, block, i);
        resource.ops[1] = FindOrAddOperandConstant(*metadataBlock, block, program.GetConstants().FindConstantOrAdd(handle.type, Backend::IL::UndefConstant{}));
        resource.ops[2] = FindOrAddString(*metadataBlock, block, handle.name);
        resource.ops[3] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.bindSpace);
        resource.ops[4] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerBase);
        resource.ops[5] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerRange);
        resource.ops[6] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(handle.uav.shape));
        resource.ops[7] = FindOrAddOperandBoolConstant(*metadataBlock, block, false);
        resource.ops[8] = FindOrAddOperandBoolConstant(*metadataBlock, block, false);
        resource.ops[9] = FindOrAddOperandBoolConstant(*metadataBlock, block, false);
        resource.ops[10] = extendedMdIndex;
        block->AddRecord(resource);

        // Create resource metadata row
        Metadata& resourceMd = metadataBlock->metadata.emplace_back();
        resourceMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Insert handle
        classRecord->ops[i] = metadataBlock->metadata.size();
    }
}

void DXILPhysicalBlockMetadata::CompileCBVResourceClass(const DXJob &job) {
    // Get mapped
    MappedRegisterClass& mapped = FindOrAddRegisterClass(DXILShaderResourceClass::CBVs);

    // None to emit?
    if (mapped.handles.empty()) {
        return;
    }

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(resources.uid);

    // Get the block
    LLVMBlock* block = declarationBlock->GetBlockWithUID(resources.uid);

    // Compile the record
    LLVMRecordView classRecord = CompileResourceClassRecord(mapped);

    // Populate handles
    for (uint32_t i = 0; i < static_cast<uint32_t>(mapped.handles.size()); i++) {
        const HandleEntry& handle = handles[mapped.handles[i]];

        // Parsed handle?
        if (handle.record) {
            continue;
        }

        // Insert resource record node
        LLVMRecord resource(LLVMMetadataRecord::Node);
        resource.SetUser(false, ~0u, ~0u);
        resource.opCount = 8;
        resource.ops = table.recordAllocator.AllocateArray<uint64_t>(resource.opCount);
        resource.ops[0] = FindOrAddOperandU32Constant(*metadataBlock, block, i);
        resource.ops[1] = FindOrAddOperandConstant(*metadataBlock, block, program.GetConstants().FindConstantOrAdd(handle.type, Backend::IL::UndefConstant{}));
        resource.ops[2] = FindOrAddString(*metadataBlock, block, handle.name);
        resource.ops[3] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.bindSpace);
        resource.ops[4] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerBase);
        resource.ops[5] = FindOrAddOperandU32Constant(*metadataBlock, block, handle.registerRange);
        resource.ops[6] = FindOrAddOperandU32Constant(*metadataBlock, block, static_cast<uint32_t>(GetPODNonAlignedTypeByteSize(handle.type->As<Backend::IL::PointerType>()->pointee)));
        resource.ops[7] = 0u;
        block->AddRecord(resource);

        // Create resource metadata row
        Metadata& resourceMd = metadataBlock->metadata.emplace_back();
        resourceMd.source = static_cast<uint32_t>(block->records.Size()) - 1;

        // Insert handle
        classRecord->ops[i] = metadataBlock->metadata.size();
    }
}

DXILPhysicalBlockMetadata::MappedRegisterClass &DXILPhysicalBlockMetadata::FindOrAddRegisterClass(DXILShaderResourceClass _class) {
    for (MappedRegisterClass& space : registerClasses) {
        if (space._class == _class) {
            return space;
        }
    }

    MappedRegisterClass& space = registerClasses.emplace_back();
    space._class = _class;
    return space;
}

DXILPhysicalBlockMetadata::UserRegisterSpace &DXILPhysicalBlockMetadata::FindOrAddRegisterSpace(uint32_t space) {
    for (UserRegisterSpace& registerSpace : registerSpaces) {
        if (registerSpace.space == space) {
            return registerSpace;
        }
    }

    registerSpaceBound = std::max<uint32_t>(registerSpaceBound, space + 1);

    UserRegisterSpace& registerSpace = registerSpaces.emplace_back();
    registerSpace.space = space;
    return registerSpace;
}

void DXILPhysicalBlockMetadata::CompileProgramEntryPoints() {
    LLVMBlock* mdBlock = declarationBlock->GetBlockWithUID(entryPoint.uid);

    // Get the metadata
    MetadataBlock* metadataBlock = GetMetadataBlock(entryPoint.uid);

    // Get the program block
    LLVMRecordView programRecord(mdBlock, entryPoint.program);

    // Copy info to binding
    table.bindingInfo.shaderFlags = programMetadata.internalShaderFlags;

    // Unbound kv node?
    if (!programRecord->Op(4)) {
        // Create KV node
        LLVMRecord kvRecord(LLVMMetadataRecord::Node);
        kvRecord.opCount = 0;
        mdBlock->AddRecord(kvRecord);

        // KV identifier
        Metadata& uavMd = metadataBlock->metadata.emplace_back();
        uavMd.source = static_cast<uint32_t>(mdBlock->records.Size()) - 1;
        programRecord->Op(4) = uavMd.source + 1;
    }

    // Get the kv node
    LLVMRecordView kvRecord(mdBlock, programRecord->Op32(4) - 1);

    // Parse tags
    for (uint32_t kv = 0; kv < kvRecord->opCount; kv += 2) {
        switch (GetOperandU32Constant<DXILProgramTag>(*metadataBlock, kvRecord->Op32(kv + 0))) {
            default: {
                break;
            }
            case DXILProgramTag::ShaderFlags: {
                // Get current flags
                uint32_t existingFlags = GetOperandU32Constant(*metadataBlock, kvRecord->Op32(kv + 1));

                // Or flags
                uint32_t combined = FindOrAddOperandU32Constant(*metadataBlock, mdBlock, existingFlags | static_cast<uint32_t>(programMetadata.internalShaderFlags.value));

                // Write combined
                kvRecord->Op(kv + 1) = combined;

                // OK
                programMetadata.internalShaderFlags = {};
                break;
            }
        }
    }

    // Pending flags?
    if (programMetadata.internalShaderFlags.value) {
        // Copy ops
        auto ops = table.recordAllocator.AllocateArray<uint64_t>(kvRecord->opCount + 2);
        std::memcpy(ops, kvRecord->ops, sizeof(uint64_t) * kvRecord->opCount);

        // Append flag
        ops[kvRecord->opCount + 0] = FindOrAddOperandU32Constant(*metadataBlock, mdBlock, static_cast<uint64_t>(DXILProgramTag::ShaderFlags));
        ops[kvRecord->opCount + 1] = FindOrAddOperandU32Constant(*metadataBlock, mdBlock, static_cast<uint32_t>(programMetadata.internalShaderFlags.value));

        // Set new ops
        kvRecord->ops = ops;
        kvRecord->opCount += 2;
    }
}

void DXILPhysicalBlockMetadata::StitchMetadataAttachments(struct LLVMBlock *block, const TrivialStackVector<uint32_t, 512>& recordRelocation) {
    MetadataBlock* mdBlock = &metadataBlocks[0];

    for (LLVMRecord& record : block->records) {
        switch (static_cast<LLVMMetadataRecord>(record.id)) {
            default: {
                break;
            }
            case LLVMMetadataRecord::Attachment: {
                // Function attachment?
                if (!(record.opCount % 2)) {
                    continue;
                }

                // Stitch record index
                record.Op(0) = recordRelocation[record.Op(0)];
                ASSERT(record.Op(0) != IL::InvalidID, "Cannot stitch metadata attachment for non-existent relocation");

                // Stitch metadata blocks
                for (uint32_t i = 1; i < record.opCount; i += 2) {
                    record.Op(i + 1) = mdBlock->sourceMappings[record.Op(i + 1) - 1] + 1;
                }
                break;
            }
        }
    }
}
