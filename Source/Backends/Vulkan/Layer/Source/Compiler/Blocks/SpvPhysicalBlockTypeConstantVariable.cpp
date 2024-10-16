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

#include "Backend/IL/ConstantKind.h"
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockTypeConstantVariable.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>
#include <Backends/Vulkan/Compiler/SpvRecordReader.h>
#include <Backends/Vulkan/Config.h>

// Backend
#include <Backend/IL/TypeSize.h>

// Common
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Sink.h>

SpvPhysicalBlockTypeConstantVariable::SpvPhysicalBlockTypeConstantVariable(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    SpvPhysicalBlockSection(allocators, program, table),
    typeMap(allocators, &program.GetTypeMap()),
    constantMap(program.GetConstants(), typeMap) {

}

void SpvPhysicalBlockTypeConstantVariable::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::TypeConstantVariable);

    // Compile to a record block
    SpvParseContext recordCtx(block->source);
    while (recordCtx) {
        recordBlock.records.push_back(SpvRecord {
            .lowWordCountHighOpCode = recordCtx->lowWordCountHighOpCode,
            .operands = &recordCtx->Word(1)
        });
        
        // Next instruction
        recordCtx.Next();
    }

    // Current anchor
    uint32_t anchor{0};
    
    // Parse instructions
    for (const SpvRecord& record : recordBlock.records) {
        SpvRecordReader ctx(record);
        
        // Create type association
        AssignTypeAssociation(ctx);

        // Handle instruction
        switch (ctx.GetOp()) {
            default:
                break;

            case SpvOpTypeInt: {
                Backend::IL::IntType type;
                type.bitWidth = ctx++;
                type.signedness = ctx++;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeVoid: {
                Backend::IL::VoidType type;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeBool: {
                Backend::IL::BoolType type;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeFloat: {
                Backend::IL::FPType type;
                type.bitWidth = ctx++;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeVector: {
                Backend::IL::VectorType type;
                type.containedType = typeMap.GetTypeFromId(ctx++);
                type.dimension = ctx++;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeMatrix: {
                const Backend::IL::Type *columnType = typeMap.GetTypeFromId(ctx++);

                auto *columnVector = columnType->Cast<Backend::IL::VectorType>();
                ASSERT(columnVector, "Column type must be vector");

                Backend::IL::MatrixType type;
                type.containedType = columnVector->containedType;
                type.rows = columnVector->dimension;
                type.columns = ctx++;
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypePointer: {
                Backend::IL::PointerType type;
                type.addressSpace = Translate(static_cast<SpvStorageClass>(ctx++));
                type.pointee = typeMap.GetTypeFromId(ctx++);

                // Validate the (potential) forward declaration matches the actual declaration
                if (const Backend::IL::Type* declared = typeMap.GetTypeFromId(ctx.GetResult())) {
                    const auto* declaredPtr = declared->Cast<Backend::IL::PointerType>();
                    ASSERT(declaredPtr && declaredPtr->addressSpace == type.addressSpace, "Misformed forward declaration");
                }
                
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeArray: {
                Backend::IL::ArrayType type;
                type.elementType = typeMap.GetTypeFromId(ctx++);
                type.count = static_cast<uint32_t>(program.GetConstants().GetConstant(ctx++)->As<IL::IntConstant>()->value);

                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeRuntimeArray: {
                Backend::IL::ArrayType type;
                type.elementType = typeMap.GetTypeFromId(ctx++);
                type.count = 0;

                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeSampler: {
                typeMap.AddType(ctx.GetResult(), anchor, Backend::IL::SamplerType{});
                break;
            }

            case SpvOpTypeSampledImage: {
                // Underlying image
                const Backend::IL::Type *imageType = typeMap.GetTypeFromId(ctx++);

                // Set to image
                typeMap.AddMapping(ctx.GetResult(), imageType);
                break;
            }

            case SpvOpTypeImage: {
                // Sampled type
                const Backend::IL::Type *sampledType = typeMap.GetTypeFromId(ctx++);

                // Dimension of the image
                SpvDim dim = static_cast<SpvDim>(ctx++);

                // Cap operands
                bool isDepth = ctx++;
                bool isArrayed = ctx++;
                bool multisampled = ctx++;

                // Ignored for now
                GRS_SINK(isDepth);

                // Translate the sampler mode
                Backend::IL::ResourceSamplerMode samplerMode;
                switch (ctx++) {
                    default:
                        ASSERT(false, "Unknown sampler mode");
                        samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                        break;
                    case 0:
                        samplerMode = Backend::IL::ResourceSamplerMode::RuntimeOnly;
                        break;
                    case 1:
                        samplerMode = Backend::IL::ResourceSamplerMode::Compatible;
                        break;
                    case 2:
                        samplerMode = Backend::IL::ResourceSamplerMode::Writable;
                        break;
                }

                // Format, if present
                Backend::IL::Format format = Translate(static_cast<SpvImageFormat>(ctx++));

                // Texel buffer?
                if (dim == SpvDimBuffer) {
                    Backend::IL::BufferType type;
                    type.elementType = sampledType;
                    type.texelType = format;
                    type.samplerMode = samplerMode;
                    typeMap.AddType(ctx.GetResult(), anchor, type);
                } else {
                    Backend::IL::TextureType type;
                    type.sampledType = sampledType;
                    type.dimension = Translate(dim);

                    if (isArrayed) {
                        switch (type.dimension) {
                            default:
                                type.dimension = Backend::IL::TextureDimension::Unexposed;
                                break;
                            case Backend::IL::TextureDimension::Texture1D:
                                type.dimension = Backend::IL::TextureDimension::Texture1DArray;
                                break;
                            case Backend::IL::TextureDimension::Texture2D:
                                type.dimension = Backend::IL::TextureDimension::Texture2DArray;
                                break;
                            case Backend::IL::TextureDimension::Texture2DCube:
                                type.dimension = Backend::IL::TextureDimension::Texture2DCubeArray;
                                break;
                        }
                    }

                    type.multisampled = multisampled;
                    type.samplerMode = samplerMode;
                    type.format = format;

                    typeMap.AddType(ctx.GetResult(), anchor, type);
                }
                break;
            }

            case SpvOpTypeFunction: {
                Backend::IL::FunctionType function;

                // Return type
                function.returnType = typeMap.GetTypeFromId(ctx++);

                // Parameter types
                while (ctx.HasPendingWords()) {
                    function.parameterTypes.push_back(typeMap.GetTypeFromId(ctx++));
                }

                typeMap.AddType(ctx.GetResult(), anchor, function);
                break;
            }

            case SpvOpTypeStruct: {
                Backend::IL::StructType _struct;

                while (ctx.HasPendingWords()) {
                    _struct.memberTypes.push_back(typeMap.GetTypeFromId(ctx++));
                }

                typeMap.AddType(ctx.GetResult(), anchor, _struct);
                break;
            }
            
            case SpvOpTypeForwardPointer: {
                SpvId forwardId = ctx++;
                
                typeMap.AddType(forwardId, anchor, Backend::IL::PointerType {
                    .pointee = nullptr,
                    .addressSpace = Translate(static_cast<SpvStorageClass>(ctx++))
                });
                break;
            }

            case SpvOpTypeEvent:
            case SpvOpTypeDeviceEvent:
            case SpvOpTypeReserveId:
            case SpvOpTypeQueue:
            case SpvOpTypePipe:
            case SpvOpTypeOpaque: {
                typeMap.AddType(ctx.GetResult(), anchor, Backend::IL::UnexposedType{});
                break;
            }

            // TODO[spec]: Derive actual specialization during instrumentation
            case SpvOpSpecConstantTrue: 
            case SpvOpConstantTrue: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Add constant
                constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                    .value = true
                });
                break;
            }

            // TODO[spec]: Derive actual specialization during instrumentation
            case SpvOpSpecConstantFalse: 
            case SpvOpConstantFalse: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Add constant
                constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                    .value = false
                });
                break;
            }

            case SpvOpConstantNull: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Add constant
                switch (type->kind) {
                    default: {
                        constantMap.AddUnsortedConstant(ctx.GetResult(), type, Backend::IL::UnexposedConstant {});
                        break;
                    }
                    case Backend::IL::TypeKind::Bool: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                            .value = 0
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Int: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                            .value = 0
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::FP: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                            .value = 0.0
                        });
                        break;
                    }
                }
                break;
            }

            // TODO[spec]: Derive actual specialization during instrumentation
            case SpvOpSpecConstant:
            case SpvOpConstant: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Add constant
                switch (type->kind) {
                    default: {
                        constantMap.AddUnsortedConstant(ctx.GetResult(), type, Backend::IL::UnexposedConstant {});
                        break;
                    }
                    case Backend::IL::TypeKind::Bool: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                            .value = static_cast<bool>(ctx++)
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Int: {
                        // Get type
                        const auto* _type = type->As<Backend::IL::IntType>();

                        // Read chunks
                        int64_t value = 0;
                        for (uint32_t i = 0; i < _type->bitWidth; i += 32) {
                            value |= (ctx++) << (32 * i);
                        }
                        
                        constantMap.AddConstant(ctx.GetResult(), _type, Backend::IL::IntConstant {
                            .value = value
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::FP: {
                        // Get type
                        const auto* _type = type->As<Backend::IL::FPType>();

                        // Read chunks
                        uint64_t value = 0;
                        for (uint32_t i = 0; i < _type->bitWidth; i += 32) {
                            value |= (ctx++) << (32 * i);
                        }
                        
                        constantMap.AddConstant(ctx.GetResult(), _type, Backend::IL::FPConstant {
                            .value = std::bit_cast<double>(value)
                        });
                        break;
                    }
                }
                break;
            }

            // TODO[spec]: Derive actual specialization during instrumentation
            case SpvOpSpecConstantComposite:
            case SpvOpConstantComposite: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Regardless of composite setup, it's just a set of constants
                std::vector<const IL::Constant*> constants;
                constants.reserve(ctx.GetWordCount() - 2);

                // Fill all constants
                while (ctx.HasPendingWords()) {
                    constants.push_back(program.GetConstants().GetConstant(ctx++));
                }

                switch (type->kind) {
                    default: {
                        constantMap.AddUnsortedConstant(ctx.GetResult(), type, Backend::IL::UnexposedConstant {});
                        break;
                    }
                    case Backend::IL::TypeKind::Array: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::ArrayType>(), Backend::IL::ArrayConstant {
                            .elements = std::move(constants)
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Vector: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::VectorType>(), Backend::IL::VectorConstant {
                            .elements = std::move(constants)
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Struct: {
                        constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::StructType>(), Backend::IL::StructConstant {
                            .members = std::move(constants)
                        });
                        break;
                    }
                }
                break;
            }

            case SpvOpVariable: {
                auto storageClass = static_cast<SpvStorageClass>(ctx++);

                // Add variable
                program.GetVariableList().Add(new (allocators) Backend::IL::Variable {
                    .id = ctx.GetResult(),
                    .addressSpace = Translate(storageClass),
                    .type = typeMap.GetTypeFromId(ctx.GetResultType())
                });
                
                // Store single PC block
                if (storageClass == SpvStorageClassPushConstant) {
                    pushConstantVariableOffset = anchor;
                    pushConstantVariableId = ctx.GetResult();
                }
                break;
            }

            case SpvOpExtInst: {
                uint32_t set = ctx.Peek();

                // Parse debug metadata
                if (table.shaderDebug.IsDebug100(set)) {
                    table.shaderDebug.ParseDebug100Instruction(ctx);
                }
                break;
            }
        }

        // Next anchor
        anchor++;
    }
}

void SpvPhysicalBlockTypeConstantVariable::Specialize(const SpvJob &job) {
    // Specialize variables based on their actual pipeline signature
    for (const Backend::IL::Variable* variable : program.GetVariableList()) {
        if (!table.annotation.IsDecorated(variable->id)) {
            continue;
        }

        // Get the decoration
        const SpvValueDecoration& decoration = table.annotation.GetDecoration(variable->id);

        // May not be found in the physical layout
        if (decoration.descriptorSet >= job.instrumentationKey.physicalMapping->descriptorSets.size()) {
            continue;
        }
        
        // Get the physical mapping
        const DescriptorLayoutPhysicalMapping& descriptorSetPhysicalMapping = job.instrumentationKey.physicalMapping->descriptorSets.at(decoration.descriptorSet);
        
        // Binding may not exist
        if (decoration.descriptorOffset >= descriptorSetPhysicalMapping.bindings.size()) {
            continue;
        }

        // Specialize per binding
        const BindingPhysicalMapping& binding = descriptorSetPhysicalMapping.bindings.at(decoration.descriptorOffset);
        switch (binding.type) {
            default: {
                // Nothing to specialize for
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                // SPIRV does not differentiate between storage and uniform buffers
                // which can make some instrumentation impossible to figure out, so,
                // help shaders a little by representing them as buffers. Also brings
                // things in line with DXIL.

                // Get the SB type
                const Backend::IL::Type *pointee = variable->type->As<Backend::IL::PointerType>()->pointee;

                // Represent the variable
                const Backend::IL::Type* type = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType {
                    .pointee = program.GetTypeMap().FindTypeOrAdd(Backend::IL::BufferType {
                        .elementType = pointee,
                        .samplerMode = Backend::IL::ResourceSamplerMode::Writable,
                        .texelType = Backend::IL::Format::None
                    }),
                    .addressSpace = Backend::IL::AddressSpace::Resource
                });

                // Modify types
                program.GetTypeMap().SetType(variable->id, type);
                break;
            }
        }
    }
}

void SpvPhysicalBlockTypeConstantVariable::AssignTypeAssociation(const SpvParseContext &ctx) {
    // If there's an associated type, map it
    if (!ctx.HasResult() || !ctx.HasResultType()) {
        return;
    }

    // Get type, if not found assume unexposed
    const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());
    if (!type) {
        type = typeMap.AddType(ctx.GetResultType(),  IL::InvalidOffset, Backend::IL::UnexposedType{});
    }

    // Create type -> id mapping
    program.GetTypeMap().SetType(ctx.GetResult(), type);
}

void SpvPhysicalBlockTypeConstantVariable::AssignTypeAssociation(const SpvRecordReader &ctx) {
    // If there's an associated type, map it
    if (!ctx.HasResult() || !ctx.HasResultType()) {
        return;
    }

    // Get type, if not found assume unexposed
    const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());
    if (!type) {
        type = typeMap.AddType(ctx.GetResultType(),  IL::InvalidOffset, Backend::IL::UnexposedType{});
    }

    // Create type -> id mapping
    program.GetTypeMap().SetType(ctx.GetResult(), type);
}

void SpvPhysicalBlockTypeConstantVariable::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockTypeConstantVariable &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::TypeConstantVariable);
    out.recordBlock = recordBlock;
    out.pushConstantBlockType = pushConstantBlockType;
    out.pushConstantVariableOffset = pushConstantVariableOffset;
    out.pushConstantVariableId = pushConstantVariableId;
    typeMap.CopyTo(out.typeMap);
    constantMap.CopyTo(out.constantMap);
}

IL::ID SpvPhysicalBlockTypeConstantVariable::CreatePushConstantBlock(const SpvJob& job) {
    // Get data map
    IL::ShaderDataMap& shaderDataMap = program.GetShaderDataMap();
    
    // Get IL map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();
    
    // Requested dword count
    uint32_t dwordCount = 0;

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    if (job.requiresUserDescriptorMapping) {
        dwordCount++;
    }
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type == ShaderDataType::Event) {
            dwordCount++;
        }
    }

    // Early out if no requests
    if (!dwordCount) {
        return IL::InvalidID;
    }
    
    // Final declaration type
    Backend::IL::StructType structDecl;

    // Identifiers
    IL::ID pcBlockPtrId;

    // Given decoration, if present
    const SpvValueDecoration* sourceDecoration{nullptr};

    // Existing PC block?
    if (pushConstantVariableOffset != IL::InvalidOffset) {
        // Get types
        const Backend::IL::Type* pcVarType = program.GetTypeMap().GetType(pushConstantVariableId);
        const auto* pointeeType = pcVarType->As<Backend::IL::PointerType>();
        const auto* structType = pointeeType->pointee->As<Backend::IL::StructType>();

        // Get decoration
        sourceDecoration = &table.annotation.GetDecoration(structType->id);

        // Reuse previous id
        pcBlockPtrId = typeMap.GetSpvTypeId(pointeeType);

        // Add old types
        for (const Backend::IL::Type* memberType : structType->memberTypes) {
            structDecl.memberTypes.push_back(memberType);
        }
    } else {
        // Allocate new ids
        pcBlockPtrId           = table.scan.header.bound++;
        pushConstantVariableId = table.scan.header.bound++;

        // Add to all entry points
        table.entryPoint.AddInterface(SpvStorageClassPushConstant, pushConstantVariableId);
    }

    // Starting offset for instrumentation
    pushConstantMemberOffset = static_cast<uint32_t>(structDecl.memberTypes.size());

    // Validate offsets
#if PRMT_METHOD == PRMT_METHOD_UB_PC
    uint32_t sourceByteOffset = job.instrumentationKey.pipelineLayoutPRMTPCOffset;
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
    uint32_t sourceByteOffset = job.instrumentationKey.pipelineLayoutDataPCOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    // UInt32
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // Add instrumented data
    for (uint32_t i = 0; i < dwordCount; i++) {
        structDecl.memberTypes.push_back(intType);
    }

    // Create type
    pushConstantBlockType = ilTypeMap.AddType(program.GetIdentifierMap().AllocID(), structDecl);
    
    // SpvIds
    SpvId pcBlockTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(pushConstantBlockType);

    // Create new pointer
    SpvInstruction &spvPtrType = table.typeConstantVariable.block->stream.Allocate(SpvOpTypePointer, 4);
    spvPtrType[1] = pcBlockPtrId;
    spvPtrType[2] = SpvStorageClassPushConstant;
    spvPtrType[3] = pcBlockTypeId;
    
    // Declare push constant block
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = pcBlockPtrId;
    spvCounterVar[2] = pushConstantVariableId;
    spvCounterVar[3] = SpvStorageClassPushConstant;

    // Block annotation
    SpvInstruction &pcBlock = table.annotation.block->stream.Allocate(SpvOpDecorate, 3);
    pcBlock[1] = pcBlockTypeId;
    pcBlock[2] = SpvDecorationBlock;

    // Migrate previous decorations if present
    if (sourceDecoration) {
        for (uint32_t i = 0; i < sourceDecoration->members.size(); i++) {
            const SpvMemberDecoration& member = sourceDecoration->members[i];

            // Emit all original pairs
            for (const SpvDecorationPair& pair : member.decorations) {
                SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpMemberDecorate, 4 + pair.wordCount);
                pcBlockMember[1] = pcBlockTypeId;
                pcBlockMember[2] = i;
                pcBlockMember[3] = pair.kind;
                std::memcpy(&pcBlockMember[4], pair.words, sizeof(uint32_t) * pair.wordCount);
            }
        }
    }

    // Block annotation
    for (uint32_t i = 0; i < dwordCount; i++) {
        SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpMemberDecorate, 5);
        pcBlockMember[1] = pcBlockTypeId;
        pcBlockMember[2] = pushConstantMemberOffset + i;
        pcBlockMember[3] = SpvDecorationOffset;
        pcBlockMember[4] = sourceByteOffset + sizeof(uint32_t) * i;
    }

    // OK
    return pushConstantVariableId;
}

IL::ID SpvPhysicalBlockTypeConstantVariable::FindOrCreateInput(SpvBuiltIn builtin, const Backend::IL::Type* type) {
    if (IL::ID value = table.annotation.GetBuiltin(builtin); value != IL::InvalidID) {
        return value;
    }

    // Create pointer to value type
    type = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType {
        .pointee = type,
        .addressSpace = Backend::IL::AddressSpace::Input
    });

    // Allocate identifiers
    IL::ID value = table.scan.header.bound++;

    // Get type identifiers
    SpvId typeId = table.typeConstantVariable.typeMap.GetSpvTypeId(type);

    // Create variable
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = typeId;
    spvCounterVar[2] = value;
    spvCounterVar[3] = SpvStorageClassInput;

    // Mark as builtin
    SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    pcBlockMember[1] = value;
    pcBlockMember[2] = SpvDecorationBuiltIn;
    pcBlockMember[3] = builtin;

    // Keep record of builtin and add to entrypoint interface list
    table.annotation.SetBuiltin(builtin, value);
    table.entryPoint.AddInterface(SpvStorageClassInput, value);
    return value;
}

void SpvPhysicalBlockTypeConstantVariable::Compile(SpvIdMap &idMap) {
    // Deprecate push constant data
    if (pushConstantVariableOffset != IL::InvalidOffset) {
        const auto* pointeeType = program.GetTypeMap().GetType(pushConstantVariableId)->As<Backend::IL::PointerType>();
        
        // Get records
        SpvRecord& varRecord = recordBlock.records.at(pushConstantVariableOffset);
        SpvRecord& ptrRecord = recordBlock.records.at(pointeeType->sourceOffset);
        
        // Deprecate old records
        varRecord.Deprecate();
        ptrRecord.Deprecate();
    }

    // Rewrite from record block
    block->stream.Clear();
    recordBlock.Write(block->stream);
    
    // Set the destination declaration stream
    typeMap.SetDeclarationStream(&block->stream);
    constantMap.SetDeclarationStream(&block->stream);

    // Set the identifier proxy map
    typeMap.SetIdMap(&idMap);
}

void SpvPhysicalBlockTypeConstantVariable::CompileConstants() {
    // Ensure all IL constants are mapped
    for (const Backend::IL::Constant* constant : program.GetConstants()) {
        if (constant->IsSymbolic()) {
            continue;
        }

        constantMap.EnsureConstant(constant);
    }
}
