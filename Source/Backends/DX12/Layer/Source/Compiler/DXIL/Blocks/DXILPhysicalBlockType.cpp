#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockType.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/DXILType.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockType::DXILPhysicalBlockType(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table) :
    DXILPhysicalBlockSection(allocators, program, table),
    typeMap(program.GetTypeMap(), program.GetIdentifierMap()) {
}

void DXILPhysicalBlockType::ParseType(const LLVMBlock *block) {
    // Visit type records
    for (const LLVMRecord &record: block->records) {
        switch (record.As<LLVMTypeRecord>()) {
            default: {
                break;
            }
            case LLVMTypeRecord::NumEntry: {
                typeMap.SetEntryCount(record.ops[0]);
                break;
            }
            case LLVMTypeRecord::Void: {
                typeMap.AddType(typeAlloc++, Backend::IL::VoidType{});
                break;
            }
            case LLVMTypeRecord::Half: {
                typeMap.AddType(typeAlloc++, Backend::IL::FPType{
                    .bitWidth = 16
                });
                break;
            }
            case LLVMTypeRecord::Float: {
                typeMap.AddType(typeAlloc++, Backend::IL::FPType{
                    .bitWidth = 32
                });
                break;
            }
            case LLVMTypeRecord::Double: {
                typeMap.AddType(typeAlloc++, Backend::IL::FPType{
                    .bitWidth = 64
                });
                break;
            }
            case LLVMTypeRecord::Integer: {
                const auto bitWidth = static_cast<uint8_t>(record.ops[0]);

                if (bitWidth == 1) {
                    typeMap.AddType(typeAlloc++, Backend::IL::BoolType{ });
                } else {
                    typeMap.AddType(typeAlloc++, Backend::IL::IntType{
                        .bitWidth = bitWidth
                    });
                }
                break;
            }
            case LLVMTypeRecord::Pointer: {
                // Get pointee
                const Backend::IL::Type *pointee = typeMap.GetType(record.ops[0]);
                if (!pointee) {
                    ASSERT(false, "Invalid pointer record");
                    return;
                }

                // Default space
                Backend::IL::AddressSpace space = Backend::IL::AddressSpace::Function;

                // Has address space?
                if (record.opCount > 1) {
                    // Translate address space
                    switch (record.OpAs<DXILAddressSpace>(1)) {
                        default:
                            break;
                        case DXILAddressSpace::Local:
                            space = Backend::IL::AddressSpace::Function;
                            break;
                        case DXILAddressSpace::Device:
                            space = Backend::IL::AddressSpace::Resource;
                            break;
                        case DXILAddressSpace::Constant:
                            space = Backend::IL::AddressSpace::Constant;
                            break;
                        case DXILAddressSpace::GroupShared:
                            space = Backend::IL::AddressSpace::GroupShared;
                            break;
                    }
                }

                // Add type
                typeMap.AddType(typeAlloc++, Backend::IL::PointerType{
                    .pointee = pointee,
                    .addressSpace = space
                });
                break;
            }
            case LLVMTypeRecord::Array: {
                // Get contained
                const Backend::IL::Type *contained = typeMap.GetType(record.ops[1]);
                if (!contained) {
                    ASSERT(false, "Invalid array record");
                    return;
                }

                // Add type
                typeMap.AddType(typeAlloc++, Backend::IL::ArrayType{
                    .elementType = contained,
                    .count = record.OpAs<uint32_t>(0)
                });
                break;
            }
            case LLVMTypeRecord::Vector: {
                // Get contained
                const Backend::IL::Type *contained = typeMap.GetType(record.ops[1]);
                if (!contained) {
                    ASSERT(false, "Invalid array record");
                    return;
                }

                // Add type
                typeMap.AddType(typeAlloc++, Backend::IL::VectorType{
                    .containedType = contained,
                    .dimension = record.OpAs<uint8_t>(0)
                });
                break;
            }
            case LLVMTypeRecord::Label: {
                typeMap.AddUnsortedType(typeAlloc++, DXILType {
                    .type = LLVMTypeRecord::Label
                });
                break;
            }
            case LLVMTypeRecord::Opaque: {
                typeMap.AddUnsortedType(typeAlloc++, DXILType {
                    .type = LLVMTypeRecord::Opaque
                });
                break;
            }
            case LLVMTypeRecord::MetaData: {
                typeMap.AddUnsortedType(typeAlloc++, DXILType {
                    .type = LLVMTypeRecord::MetaData
                });
                break;
            }
            case LLVMTypeRecord::StructAnon: {
                typeMap.AddUnsortedType(typeAlloc++, DXILType {
                    .type = LLVMTypeRecord::StructAnon
                });
                break;
            }
            case LLVMTypeRecord::StructName: {
                /* Ignore */
                break;
            }
            case LLVMTypeRecord::StructNamed: {
                typeMap.AddUnsortedType(typeAlloc++, DXILType {
                    .type = LLVMTypeRecord::StructNamed
                });
                break;
            }
            case LLVMTypeRecord::Function: {
                ASSERT(record.Op(0) == 0, "VaArg functions not supported");

                Backend::IL::FunctionType decl;
                decl.returnType = typeMap.GetType(record.Op(1));

                // Preallocate
                decl.parameterTypes.reserve(record.opCount - 2);

                // Fill parameters
                for (uint32_t i = 2; i < record.opCount; i++) {
                    decl.parameterTypes.push_back(typeMap.GetType(record.Op(i)));
                }

                typeMap.AddType(typeAlloc++, decl);
                break;
            }
        }
    }
}

void DXILPhysicalBlockType::CompileType(LLVMBlock *block) {
    // Compile all missing types
    for (Backend::IL::Type* type : program.GetTypeMap()) {
        if (table.type.typeMap.HasType(type)) {
            continue;
        }

        LLVMRecord record{};

        bool isMetaType = false;

        // Attempt to compile the type, not all types are supported for compilation
        switch (type->kind) {
            default:
                ASSERT(false, "Invalid type for DXIL compilation");
                break;
            case Backend::IL::TypeKind::Unexposed:
            case Backend::IL::TypeKind::Buffer:
            case Backend::IL::TypeKind::Texture: {
                isMetaType = true;
                break;
            }
            case Backend::IL::TypeKind::Void: {
                record.id = static_cast<uint32_t>(LLVMTypeRecord::Void);
                break;
            }
            case Backend::IL::TypeKind::Bool:
            case Backend::IL::TypeKind::Int: {
                record.id = static_cast<uint32_t>(LLVMTypeRecord::Integer);
                record.opCount = 1;
                record.ops = table.recordAllocator.AllocateArray<uint64_t>(1);

                if (type->kind == Backend::IL::TypeKind::Bool) {
                    record.ops[0] = 1u;
                } else {
                    record.ops[0] = type->As<Backend::IL::IntType>()->bitWidth;
                }
                break;
            }
            case Backend::IL::TypeKind::FP: {
                switch (type->As<Backend::IL::FPType>()->bitWidth) {
                    default:
                        ASSERT(false, "Invalid floating point bit-width");
                        break;
                    case 16:
                        record.id = static_cast<uint32_t>(LLVMTypeRecord::Half);
                        break;
                    case 32:
                        record.id = static_cast<uint32_t>(LLVMTypeRecord::Float);
                        break;
                    case 64:
                        record.id = static_cast<uint32_t>(LLVMTypeRecord::Double);
                        break;
                }
                break;
            }
            case Backend::IL::TypeKind::Vector: {
                auto _type = type->As<Backend::IL::VectorType>();

                record.id = static_cast<uint32_t>(LLVMTypeRecord::Vector);
                record.opCount = 2;

                record.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                record.ops[0] = _type->dimension;
                record.ops[1] = typeMap.GetType(_type->containedType);
                break;
            }
            case Backend::IL::TypeKind::Pointer: {
                auto _type = type->As<Backend::IL::PointerType>();

                record.id = static_cast<uint32_t>(LLVMTypeRecord::Pointer);
                record.opCount = 2;

                record.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                record.ops[0] = typeMap.GetType(_type->pointee);

                // Translate address space
                switch (_type->addressSpace) {
                    default:
                        ASSERT(false, "Invalid address space");
                        break;
                    case Backend::IL::AddressSpace::Constant:
                        record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Constant);
                        break;
                    case Backend::IL::AddressSpace::Function:
                        record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Local);
                        break;
                    case Backend::IL::AddressSpace::Texture:
                    case Backend::IL::AddressSpace::Buffer:
                    case Backend::IL::AddressSpace::Resource:
                        record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Device);
                        break;
                    case Backend::IL::AddressSpace::GroupShared:
                        record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::GroupShared);
                        break;
                }
                break;
            }
            case Backend::IL::TypeKind::Array: {
                auto _type = type->As<Backend::IL::ArrayType>();

                record.id = static_cast<uint32_t>(LLVMTypeRecord::Array);
                record.opCount = 2;

                record.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                record.ops[0] = _type->count;
                record.ops[1] = typeMap.GetType(_type->elementType);
                break;
            }
        }

        if (isMetaType) {
            continue;
        }

        // Add final record
        block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Record, block->records.Size()));
        block->records.Add(record);

        // Next
        typeAlloc++;
    }
}

void DXILPhysicalBlockType::CopyTo(DXILPhysicalBlockType &out) {
    out.typeAlloc = typeAlloc;

    typeMap.CopyTo(out.typeMap);
}
