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
    constantMap(allocators, program.GetConstants(), program.GetIdentifierMap(), table.type.typeMap) {

}

void DXILPhysicalBlockGlobal::ParseConstants(struct LLVMBlock *block) {
    // Current type
    const Backend::IL::Type* type{nullptr};

    // Get maps
    Backend::IL::TypeMap& types = program.GetTypeMap();

    for (LLVMRecord &record: block->records) {
        if (record.Is(LLVMConstantRecord::SetType)) {
            type = table.type.typeMap.GetType(record.ops[0]);
            continue;
        }

        // Get anchor
        uint32_t anchor = table.idMap.GetAnchor();

        // Allocate source id
        IL::ID id = table.idMap.AllocMappedID(DXILIDType::Constant);

        // Assign as source
        record.SetSource(true, anchor);

        // Final constant
        const Backend::IL::Constant* constant{nullptr};

        switch (static_cast<LLVMConstantRecord>(record.id)) {
            default: {
                ASSERT(false, "Unsupported constant record");
                return;
            }

            case LLVMConstantRecord::Null: {
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
                constant = constantMap.AddConstant(id, type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                    .value = LLVMBitStreamReader::DecodeSigned(record.Op(0))
                });
                break;
            }

            case LLVMConstantRecord::Float: {
                constant = constantMap.AddConstant(id, type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                    .value = record.OpBitCast<float>(0)
                });
                break;
            }

            case LLVMConstantRecord::Undef: {
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

void DXILPhysicalBlockGlobal::ParseGlobalVar(struct LLVMRecord& record) {
    record.SetSource(true, table.idMap.GetAnchor());
    table.idMap.AllocMappedID(DXILIDType::Variable);
}

void DXILPhysicalBlockGlobal::ParseAlias(LLVMRecord &record) {
    record.SetSource(true, table.idMap.GetAnchor());
    table.idMap.AllocMappedID(DXILIDType::Alias);
}

void DXILPhysicalBlockGlobal::CompileConstants(struct LLVMBlock *block) {

}

void DXILPhysicalBlockGlobal::CompileGlobalVar(LLVMRecord &record) {

}

void DXILPhysicalBlockGlobal::CompileAlias(LLVMRecord &record) {

}

void DXILPhysicalBlockGlobal::StitchConstants(struct LLVMBlock *block) {
    for (const LLVMRecord &record: block->records) {
        if (record.Is(LLVMConstantRecord::SetType)) {
            continue;
        }

        table.idRemapper.AllocRecordMapping(record);
    }
}

void DXILPhysicalBlockGlobal::StitchGlobalVar(LLVMRecord &record) {
    table.idRemapper.AllocRecordMapping(record);
}

void DXILPhysicalBlockGlobal::StitchAlias(LLVMRecord &record) {
    table.idRemapper.AllocRecordMapping(record);
}

void DXILPhysicalBlockGlobal::CopyTo(DXILPhysicalBlockGlobal &out) {
    constantMap.CopyTo(out.constantMap);
}
