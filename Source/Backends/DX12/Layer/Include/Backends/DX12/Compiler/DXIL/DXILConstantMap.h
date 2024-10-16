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

#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamWriter.h>
#include <Backends/DX12/Compiler/DXIL/DXILTypeMap.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDRemapper.h>

// Backend
#include <Backend/IL/ConstantMap.h>
#include <Backend/IL/IdentifierMap.h>
#include <Backend/IL/Constant.h>

// Std
#include <vector>

class DXILConstantMap {
public:
    DXILConstantMap(const Allocators& allocators, Backend::IL::ConstantMap& programMap, Backend::IL::IdentifierMap& identifierMap, DXILTypeMap& typeMap) :
        programMap(programMap),
        identifierMap(identifierMap),
        constants(allocators.Tag(kAllocModuleDXILConstants)),
        constantLookup(allocators.Tag(kAllocModuleDXILConstants)),
        typeMap(typeMap),
        recordAllocator(allocators.Tag(kAllocModuleDXILRecOps)) {

    }

    /// Copy this constant map
    /// \param out destination map
    void CopyTo(DXILConstantMap& out) {
        out.constants = constants;
        out.constantLookup = constantLookup;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant* AddConstant(IL::ID id, const typename T::Type* type, const T &constant) {
        const auto *constantPtr = programMap.AddConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant* AddUnsortedConstant(IL::ID id, const Backend::IL::Type* type, const T &constant) {
        const auto *constantPtr = programMap.AddUnsortedConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Add an unresolved constant to this map
    /// This must be resolved through ResolveConstant
    /// \param id target identifier
    /// \param type of the constant to be instantiated
    /// \param constant the constant to be added
    template<typename T>
    Backend::IL::Constant* AddUnresolvedConstant(IL::ID id, const Backend::IL::Type* type, const T &constant) {
        auto *constantPtr = programMap.AddUnresolvedConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Resolve a constant
    /// This must have been allocated through AddUnresolvedContant
    /// \param constant the constant to be resolved
    template<typename T>
    void ResolveConstant(T* constant) {
        programMap.ResolveConstant<T>(constant);
    }

    /// Get constant at offset
    /// \param id source id
    /// \return nullptr if not found
    const IL::Constant* GetConstant(uint32_t id) {
        if (id >= constants.size()) {
            return nullptr;
        }

        return constants[id];
    }

    /// Get a constant from a type
    /// \param type IL type
    /// \return DXIL id
    uint64_t GetConstant(const Backend::IL::Constant* constant) {
        // Allocate if need be
        if (!HasConstant(constant)) {
            return CompileConstant(constant);
        }

        uint64_t id = constantLookup.at(constant->id);
        ASSERT(id != ~0u, "Unallocated constant");
        return id;
    }

    /// Add a new constant mapping
    /// \param type IL type
    /// \param index DXIL id
    void AddConstantMapping(const Backend::IL::Constant* constant, uint64_t index) {
        if (constantLookup.size() <= constant->id) {
            constantLookup.resize(constant->id + 1, ~0u);
        }

        constantLookup[constant->id] = index;
    }

    /// Check if a constant is present in DXIL
    /// \param type IL type
    /// \return true if present
    bool HasConstant(const Backend::IL::Constant* type) {
        return type->id < constantLookup.size() && constantLookup[type->id] != ~0u;
    }

    /// Set the declaration block for undeclared DXIL constants
    /// \param block destination block
    void SetDeclarationBlock(LLVMBlock* block) {
        declarationBlock = block;
    }

private:
    /// Compile a given constant
    /// \param constant
    /// \return DXIL id
    uint64_t CompileConstant(const Backend::IL::Constant* constant) {
        switch (constant->kind) {
            default:
                ASSERT(false, "Unsupported constant type for recompilation");
                return ~0;
            case Backend::IL::ConstantKind::Bool:
                return CompileConstant(static_cast<const Backend::IL::BoolConstant*>(constant));
            case Backend::IL::ConstantKind::Int:
                return CompileConstant(static_cast<const Backend::IL::IntConstant*>(constant));
            case Backend::IL::ConstantKind::FP:
                return CompileConstant(static_cast<const Backend::IL::FPConstant*>(constant));
            case Backend::IL::ConstantKind::Undef:
                return CompileConstant(static_cast<const Backend::IL::UndefConstant*>(constant));
            case Backend::IL::ConstantKind::Null:
                return CompileConstant(static_cast<const Backend::IL::NullConstant*>(constant));
            case Backend::IL::ConstantKind::Struct:
                return CompileConstant(static_cast<const Backend::IL::StructConstant*>(constant));
            case Backend::IL::ConstantKind::Vector:
                return CompileConstant(static_cast<const Backend::IL::VectorConstant*>(constant));
            case Backend::IL::ConstantKind::Array:
                return CompileConstant(static_cast<const Backend::IL::ArrayConstant*>(constant));
        }
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::BoolConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = LLVMBitStreamWriter::EncodeSigned(constant->value);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::IntConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = LLVMBitStreamWriter::EncodeSigned(constant->value);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::FPConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Float);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.OpBitWrite(0, constant->value);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::UndefConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Undef);
        record.opCount = 0;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::NullConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Null);
        record.opCount = 0;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::StructConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Aggregate);
        record.opCount = static_cast<uint32_t>(constant->members.size());
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);

        for (uint32_t i = 0; i < record.opCount; i++) {
            record.ops[i] = CompileConstant(constant->members[i]);
        }
        
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::VectorConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Aggregate);
        record.opCount = static_cast<uint32_t>(constant->elements.size());
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);

        for (uint32_t i = 0; i < record.opCount; i++) {
            record.ops[i] = CompileConstant(constant->elements[i]);
        }
        
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint64_t CompileConstant(const Backend::IL::ArrayConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Aggregate);
        record.opCount = static_cast<uint32_t>(constant->elements.size());
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);

        for (uint32_t i = 0; i < record.opCount; i++) {
            record.ops[i] = CompileConstant(constant->elements[i]);
        }
        
        return Emit(constant, record);
    }

    /// Emit a constant and its respective records
    /// \param constant constant to be emitted
    /// \param record constant record
    /// \return DXIL id
    uint64_t Emit(const Backend::IL::Constant* constant, LLVMRecord& record) {
        // Insert type record prior
        LLVMRecord setTypeRecord(LLVMConstantRecord::SetType);
        setTypeRecord.opCount = 1;
        setTypeRecord.ops = recordAllocator.AllocateArray<uint64_t>(1);
        setTypeRecord.ops[0] = typeMap.GetType(constant->type);

        // Emit into block
        declarationBlock->AddRecord(setTypeRecord);

        // Add mapping
        uint64_t encodedId = DXILIDRemapper::EncodeUserOperand(constant->id);
        AddConstantMapping(constant, encodedId);

        // Always user record
        record.SetUser(true, ~0u, constant->id);

        // Emit into block
        declarationBlock->AddRecord(record);

        // OK
        return encodedId;
    }

private:
    /// IL map
    Backend::IL::ConstantMap& programMap;

    /// Identifier map
    Backend::IL::IdentifierMap& identifierMap;

    /// All constants
    Vector<const IL::Constant*> constants;

    /// IL type to DXIL type table
    Vector<uint64_t> constantLookup;

    /// Shared type map
    DXILTypeMap& typeMap;

    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

    /// Declaration block
    LLVMBlock* declarationBlock{nullptr};
};
