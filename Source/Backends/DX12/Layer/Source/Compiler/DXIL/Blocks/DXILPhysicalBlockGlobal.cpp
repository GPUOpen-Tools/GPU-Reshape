#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockGlobal.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStream.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockGlobal::ParseConstants(const struct LLVMBlock *block) {
    // Current type
    const Backend::IL::Type* type{nullptr};

    // Get maps
    Backend::IL::TypeMap& types = program.GetTypeMap();
    Backend::IL::ConstantMap& constantMap = program.GetConstants();

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
                    .value = LLVMBitStream::DecodeSigned(record.Op(0))
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

            /* Just create the mapping for now */
            case LLVMConstantRecord::Undef:
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

            // Append
            constants.push_back(constant);
        }
    }
}

void DXILPhysicalBlockGlobal::ParseGlobalVar(const struct LLVMRecord& record) {
    table.idMap.AllocMappedID(DXILIDType::Variable);
}

void DXILPhysicalBlockGlobal::ParseAlias(const LLVMRecord &record) {
    table.idMap.AllocMappedID(DXILIDType::Alias);
}
