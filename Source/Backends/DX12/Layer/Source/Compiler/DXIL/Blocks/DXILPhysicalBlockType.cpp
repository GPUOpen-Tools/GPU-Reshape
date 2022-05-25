#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockType.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockType::DXILPhysicalBlockType(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table) :
    DXILPhysicalBlockSection(allocators, program, table),
    typeMap(&program.GetTypeMap()) {
}

void DXILPhysicalBlockType::ParseType(const LLVMBlock *block) {
    // Incremented type typeAlloc++
    uint32_t typeAlloc = 0;

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
                typeMap.AddType(typeAlloc++, Backend::IL::IntType{
                    .bitWidth = static_cast<uint8_t>(record.ops[0])
                });
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
                typeMap.AddType(typeAlloc++, Backend::IL::UnexposedType{});
                break;
            }
            case LLVMTypeRecord::Opaque: {
                typeMap.AddType(typeAlloc++, Backend::IL::UnexposedType{});
                break;
            }
            case LLVMTypeRecord::MetaData: {
                typeMap.AddType(typeAlloc++, Backend::IL::UnexposedType{});
                break;
            }
            case LLVMTypeRecord::StructAnon: {
                typeMap.AddType(typeAlloc++, Backend::IL::UnexposedType{});
                break;
            }
            case LLVMTypeRecord::StructName: {
                /* Ignore */
                break;
            }
            case LLVMTypeRecord::StructNamed: {
                typeMap.AddType(typeAlloc++, Backend::IL::UnexposedType{});
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
