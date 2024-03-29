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
    typeMap(allocators, table.idRemapper, program.GetTypeMap(), program.GetIdentifierMap()) {
}

void DXILPhysicalBlockType::ParseType(const LLVMBlock *block) {
    // Names for struct consumes
    const LLVMRecord* nameConsumeRecord{nullptr};
    
    // Visit type records
    for (const LLVMRecord &record: block->records) {
        switch (record.As<LLVMTypeRecord>()) {
            default: {
                break;
            }
            case LLVMTypeRecord::NumEntry: {
                typeMap.SetEntryCount(static_cast<uint32_t>(record.ops[0]));
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
                        .bitWidth = bitWidth,
                        .signedness = true
                    });
                }
                break;
            }
            case LLVMTypeRecord::Pointer: {
                // Get pointee
                const Backend::IL::Type *pointee = typeMap.GetType(static_cast<uint32_t>(record.ops[0]));
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
                const Backend::IL::Type *contained = typeMap.GetType(static_cast<uint32_t>(record.ops[1]));
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
                const Backend::IL::Type *contained = typeMap.GetType(static_cast<uint32_t>(record.ops[1]));
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
            case LLVMTypeRecord::StructName: {
                nameConsumeRecord = &record;
                break;
            }
            case LLVMTypeRecord::StructAnon: {
                Backend::IL::StructType type;

                for (uint32_t i = 1; i < record.opCount; i++) {
                    type.memberTypes.push_back(typeMap.GetType(record.Op32(i)));
                }

                typeMap.AddType(typeAlloc++, type);
                break;
            }
            case LLVMTypeRecord::StructNamed: {
                Backend::IL::StructType type;

                for (uint32_t i = 1; i < record.opCount; i++) {
                    type.memberTypes.push_back(typeMap.GetType(record.Op32(i)));
                }

                // Temporary contiguous name
                TrivialStackVector<char, 128> name(allocators);
                name.Resize(nameConsumeRecord->opCount + 1);

                // Fill name
                for (uint32_t i = 0; i < nameConsumeRecord->opCount; i++) {
                    name[i] = static_cast<char>(nameConsumeRecord->ops[i]);
                }

                // End
                name[nameConsumeRecord->opCount] = '\0';

                typeMap.AddNamedType(typeAlloc++, type, name.Data());
                break;
            }
            case LLVMTypeRecord::Function: {
                ASSERT(record.Op(0) == 0, "VaArg functions not supported");

                Backend::IL::FunctionType decl;
                decl.returnType = typeMap.GetType(record.Op32(1));

                // Preallocate
                decl.parameterTypes.reserve(record.opCount - 2);

                // Fill parameters
                for (uint32_t i = 2; i < record.opCount; i++) {
                    decl.parameterTypes.push_back(typeMap.GetType(record.Op32(i)));
                }

                typeMap.AddType(typeAlloc++, decl);
                break;
            }
        }
    }
}

void DXILPhysicalBlockType::CompileType(LLVMBlock *block) {

}

void DXILPhysicalBlockType::CopyTo(DXILPhysicalBlockType &out) {
    out.typeAlloc = typeAlloc;

    typeMap.CopyTo(out.typeMap);
}

void DXILPhysicalBlockType::StitchType(LLVMBlock *block) {
    // Stitch entry record, must be done after compilation as types may be allocated during
    LLVMRecord& record = block->records[0];
    ASSERT(record.Is(LLVMTypeRecord::NumEntry), "Invalid type block");

    // Set new number of entries
    record.ops[0] = typeMap.GetEntryCount();
}
