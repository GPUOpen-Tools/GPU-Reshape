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
                typeMap.AddType(ctx.GetResult(), anchor, type);
                break;
            }

            case SpvOpTypeArray: {
                Backend::IL::ArrayType type;
                type.elementType = typeMap.GetTypeFromId(ctx++);
                type.count = ctx++;

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

            case SpvOpTypeEvent:
            case SpvOpTypeDeviceEvent:
            case SpvOpTypeReserveId:
            case SpvOpTypeQueue:
            case SpvOpTypePipe:
            case SpvOpTypeForwardPointer:
            case SpvOpTypeOpaque: {
                typeMap.AddType(ctx.GetResult(), anchor, Backend::IL::UnexposedType{});
                break;
            }

            case SpvOpConstantTrue: {
                const Backend::IL::Type* type = typeMap.GetTypeFromId(ctx.GetResultType());

                // Add constant
                constantMap.AddConstant(ctx.GetResult(), type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                    .value = true
                });
                break;
            }

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

            case SpvOpVariable: {
                auto storageClass = static_cast<SpvStorageClass>(ctx++);
                
                // Add variable
                program.GetVariableList().Add(Backend::IL::Variable {
                    .id = ctx.GetResult(),
                    .addressSpace = Translate(storageClass),
                    .type = program.GetTypeMap().GetType(ctx.GetResultType())
                });
                
                // Store single PC block
                if (storageClass == SpvStorageClassPushConstant) {
                    pushConstantVariableOffset = anchor;
                    pushConstantVariableId = ctx.GetResult();
                }
                break;
            }
        }

        // Next anchor
        anchor++;
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
        for (uint32_t i = 0; i < sourceDecoration->memberDecorations.size(); i++) {
            SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpMemberDecorate, 5);
            pcBlockMember[1] = pcBlockTypeId;
            pcBlockMember[2] = i;
            pcBlockMember[3] = SpvDecorationOffset;
            pcBlockMember[4] = sourceDecoration->memberDecorations[i].blockOffset;
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
        constantMap.EnsureConstant(constant);
    }
}
