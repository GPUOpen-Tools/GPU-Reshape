#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockGlobal.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamReader.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockGlobal::DXILPhysicalBlockGlobal(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table) :
    DXILPhysicalBlockSection(allocators, program, table),
    constantMap(program.GetConstants(), program.GetIdentifierMap()) {

}

void DXILPhysicalBlockGlobal::ParseConstants(const struct LLVMBlock *block) {
    // Current type
    const Backend::IL::Type* type{nullptr};

    // Get maps
    Backend::IL::TypeMap& types = program.GetTypeMap();

    for (const LLVMRecord &record: block->records) {
        uint32_t anchor = table.idMap.GetAnchor();

        // Final constant
        const Backend::IL::Constant* constant{nullptr};

        switch (static_cast<LLVMConstantRecord>(record.id)) {
            default: {
                ASSERT(false, "Unsupported constant record");
                return;
            }

            /* Set type for succeeding constants */
            case LLVMConstantRecord::SetType: {
                type = table.type.typeMap.GetType(record.ops[0]);
                break;
            }

            case LLVMConstantRecord::Null: {
                IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

                // Emit null type
                switch (type->kind) {
                    default: {
                        // Emit as unexposed
                        constant = constantMap.AddConstant(id, types.FindTypeOrAdd(Backend::IL::UnexposedType{}), Backend::IL::UnexposedConstant {});
                        break;
                    }
                    case Backend::IL::TypeKind::Bool:
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                            .value = false
                        });
                        break;
                    case Backend::IL::TypeKind::Int:
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                            .value = 0
                        });
                        break;
                    case Backend::IL::TypeKind::FP:
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                            .value = 0.0f
                        });
                        break;
                }
                break;
            }

            case LLVMConstantRecord::Integer: {
                IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

                constant = constantMap.AddConstant(id, type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                    .value = LLVMBitStreamReader::DecodeSigned(record.Op(0))
                });
                break;
            }

            case LLVMConstantRecord::Float: {
                IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

                constant = constantMap.AddConstant(id, type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                    .value = record.OpBitCast<float>(0)
                });
                break;
            }

            case LLVMConstantRecord::Undef: {
                IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

                constant = constantMap.AddUnsortedConstant(id, type, Backend::IL::UnexposedConstant {});
                break;
            }

            /* Just create the mapping for now */
            case LLVMConstantRecord::Aggregate:
            case LLVMConstantRecord::String:
            case LLVMConstantRecord::CString:
            case LLVMConstantRecord::Cast:
            case LLVMConstantRecord::GEP:
            case LLVMConstantRecord::InBoundsGEP:
            case LLVMConstantRecord::Data: {
                IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

                constant = constantMap.AddConstant(id, types.FindTypeOrAdd(Backend::IL::UnexposedType{}), Backend::IL::UnexposedConstant {});
                break;
            }
        }

        // Mapping
        if (constant) {
            // Set mapped value
            table.idMap.SetMapped(anchor, constant->id);
        }
    }
}

void DXILPhysicalBlockGlobal::ParseGlobalVar(const struct LLVMRecord& record) {
    table.idMap.AllocMappedID(DXILIDType::Variable);
}

void DXILPhysicalBlockGlobal::ParseAlias(const LLVMRecord &record) {
    table.idMap.AllocMappedID(DXILIDType::Alias);
}

void DXILPhysicalBlockGlobal::CompileConstants(struct LLVMBlock *block) {
    // Compile all missing constants
    for (Backend::IL::Constant* constant : program.GetConstants()) {
        if (constantMap.HasConstant(constant)) {
            continue;
        }

        LLVMRecord record{};

        // Attempt to compile the constant, not all types are supported for compilation
        switch (constant->type->kind) {
            default:
                ASSERT(false, "Invalid constant for DXIL compilation");
                break;
            case Backend::IL::TypeKind::Bool:
            case Backend::IL::TypeKind::Int: {
                record.id = static_cast<uint32_t>(LLVMConstantRecord::Integer);
                record.opCount = 1;

                record.ops = table.recordAllocator.AllocateArray<uint64_t>(1);

                if (constant->type->kind == Backend::IL::TypeKind::Bool) {
                    record.ops[0] = LLVMBitStreamWriter::EncodeSigned(constant->As<Backend::IL::BoolConstant>()->value);
                } else {
                    auto _constant = constant->As<Backend::IL::IntConstant>();
                    record.ops[0] = LLVMBitStreamWriter::EncodeSigned(_constant->value);
                }
                break;
            }
            case Backend::IL::TypeKind::FP: {
                auto _constant = constant->As<Backend::IL::FPConstant>();

                record.id = static_cast<uint32_t>(LLVMConstantRecord::Float);
                record.opCount = 1;

                record.ops = table.recordAllocator.AllocateArray<uint64_t>(1);
                record.OpBitWrite(0, _constant->value);
                break;
            }
        }

        // Add final record
        block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Record, block->records.Size()));
        block->records.Add(record);
    }
}

void DXILPhysicalBlockGlobal::CompileGlobalVar(LLVMRecord &record) {
    table.idRemapper.AllocSourceMapping();
}

void DXILPhysicalBlockGlobal::CompileAlias(LLVMRecord &record) {
    table.idRemapper.AllocSourceMapping();
}

void DXILPhysicalBlockGlobal::CopyTo(DXILPhysicalBlockGlobal &out) {
    constantMap.CopyTo(out.constantMap);
}
