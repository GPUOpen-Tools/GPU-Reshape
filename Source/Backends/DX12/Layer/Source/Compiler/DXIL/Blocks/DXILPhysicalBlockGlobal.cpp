// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
            type = table.type.typeMap.GetType(static_cast<uint32_t>(record.ops[0]));
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
                    case Backend::IL::TypeKind::Bool: {
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                            .value = false
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Int: {
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                            .value = 0
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::FP: {
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                            .value = 0.0
                        });
                        break;
                    }
                    case Backend::IL::TypeKind::Struct: {
                        // TODO: This is in complete disarray to the above, there's a systematic issue here
                        constant = constantMap.AddConstant(id, type->As<Backend::IL::StructType>(), Backend::IL::NullConstant {
                            
                        });
                        break;
                    }
                }
                break;
            }

            case LLVMConstantRecord::Integer: {
                if (type->Is<Backend::IL::BoolType>()) {
                    constant = constantMap.AddConstant(id, type->As<Backend::IL::BoolType>(), Backend::IL::BoolConstant {
                        .value = static_cast<bool>(LLVMBitStreamReader::DecodeSigned(record.Op(0)))
                    });
                } else {
                    constant = constantMap.AddConstant(id, type->As<Backend::IL::IntType>(), Backend::IL::IntConstant {
                        .value = LLVMBitStreamReader::DecodeSigned(record.Op(0))
                    });
                }
                break;
            }

            case LLVMConstantRecord::Float: {
                constant = constantMap.AddConstant(id, type->As<Backend::IL::FPType>(), Backend::IL::FPConstant {
                    .value = record.OpBitCast<float>(0)
                });
                break;
            }

            case LLVMConstantRecord::Aggregate: {
                if (auto _struct = type->Cast<Backend::IL::StructType>()) {
                    Backend::IL::StructConstant decl;

                    // Fill members
                    for (uint32_t i = 0; i < record.opCount; i++) {
                        decl.members.push_back(program.GetConstants().GetConstant(table.idMap.GetMapped(record.Op32(i))));
                    }
            
                    constant = constantMap.AddConstant(id, _struct, decl);
                } else {
                    // TODO: Array constants
                    constant = constantMap.AddUnsortedConstant(id, type, Backend::IL::UnexposedConstant {});
                }
                break;
            }

            case LLVMConstantRecord::Undef: {
                constant = constantMap.AddConstant(id, type, Backend::IL::UndefConstant{ });
                break;
            }

            /* Just create the mapping for now */
            case LLVMConstantRecord::String:
            case LLVMConstantRecord::CString:
            case LLVMConstantRecord::Cast:
            case LLVMConstantRecord::GEP:
            case LLVMConstantRecord::InBoundsGEP:
            case LLVMConstantRecord::Data: {
                // Emitting as unsorted is safe for DXIL resident types, as the IL has no applicable type anyway
                constant = constantMap.AddUnsortedConstant(id, type, Backend::IL::UnexposedConstant {});
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

    // Allocate
    IL::ID id = table.idMap.AllocMappedID(DXILIDType::Variable);

    // Set type, IL programs have no concept of globals for now
    const Backend::IL::Type* pointeeType = table.type.typeMap.GetType(static_cast<uint32_t>(record.ops[0]));

    // Always stored as pointer to
    const Backend::IL::Type* pointerType = program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType {
        .pointee = pointeeType,
        .addressSpace = Backend::IL::AddressSpace::Function
    });

    // Set type
    program.GetTypeMap().SetType(id, pointerType);

    // Add variable
    program.GetVariableList().Add(Backend::IL::Variable {
        .id = id,
        .addressSpace = Backend::IL::AddressSpace::Function,
        .type = pointerType
    });
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
    // Ensure all IL constants are mapped
    for (const Backend::IL::Constant* constant : program.GetConstants()) {
        constantMap.GetConstant(constant);
    }

    for (const LLVMBlockElement& element : block->elements) {
        if (!element.Is(LLVMBlockElementType::Record)) {
            continue;
        }

        LLVMRecord& record = block->records[element.id];
        if (record.Is(LLVMConstantRecord::SetType)) {
            continue;
        }

        // Disable abbreviations, stitching can potentially invalidate this
        record.abbreviation.type = LLVMRecordAbbreviationType::None;

        // Allocate result
        table.idRemapper.AllocRecordMapping(record);

        switch (static_cast<LLVMConstantRecord>(record.id)) {
            default:
                break;

            case LLVMConstantRecord::GEP: {
                break;
            }

            case LLVMConstantRecord::Aggregate: {
                for (uint32_t i = 0; i < record.opCount; i++) {
                    table.idRemapper.Remap(record.Op(i));
                }
                break;
            }

            case LLVMConstantRecord::InBoundsGEP: {
                for (uint32_t i = 1; i < record.opCount; i += 2) {
                    table.idRemapper.Remap(record.Op(i + 1));
                }
                break;
            }

            case LLVMConstantRecord::Cast: {
                table.idRemapper.Remap(record.Op(2));
                break;
            }
        }
    }
}

void DXILPhysicalBlockGlobal::StitchGlobalVar(LLVMRecord &record) {
    /*
     * LLVM Specification
     *   [GLOBALVAR, strtab offset, strtab size, pointer type, isconst, initid, linkage, alignment, section,
     *   visibility, threadlocal, unnamed_addr, externally_initialized, dllstorageclass, comdat, attributes, preemptionspecifier]
     *
     * DXC "Specification"
     *   [GLOBALVAR, type, isconst, initid, linkage, alignment, section, visibility, threadlocal,
     *   unnamed_addr, externally_initialized, dllstorageclass, comdat]
     */

    table.idRemapper.AllocRecordMapping(record);

    // Initializer?
    if (record.Op(2) > 0) {
        table.idRemapper.Remap(record.Op(2), DXILIDRemapRule::Nullable);
    }
}

void DXILPhysicalBlockGlobal::StitchAlias(LLVMRecord &record) {
    table.idRemapper.AllocRecordMapping(record);
}

void DXILPhysicalBlockGlobal::CopyTo(DXILPhysicalBlockGlobal &out) {
    constantMap.CopyTo(out.constantMap);
}
