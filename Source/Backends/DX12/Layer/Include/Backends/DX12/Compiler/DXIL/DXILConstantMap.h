#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamWriter.h>
#include <Backends/DX12/Compiler/DXIL/DXILTypeMap.h>

// Backend
#include <Backend/IL/ConstantMap.h>
#include <Backend/IL/IdentifierMap.h>

// Std
#include <vector>

class DXILConstantMap {
public:
    DXILConstantMap(const Allocators& allocators, Backend::IL::ConstantMap& programMap, Backend::IL::IdentifierMap& identifierMap, DXILTypeMap& typeMap) :
        programMap(programMap),
        identifierMap(identifierMap),
        recordAllocator(allocators),
        typeMap(typeMap) {

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
    uint32_t GetConstant(const Backend::IL::Constant* constant) {
        // Allocate if need be
        if (!HasConstant(constant)) {
            return CompileConstant(constant);
        }

        uint32_t id = constantLookup.at(constant->id);
        ASSERT(id != ~0u, "Unallocated constant");
        return id;
    }

    /// Add a new constant mapping
    /// \param type IL type
    /// \param index DXIL id
    void AddConstantMapping(const Backend::IL::Constant* type, uint32_t index) {
        if (constantLookup.size() <= type->id) {
            constantLookup.resize(type->id + 1, ~0u);
        }

        constantLookup[type->id] = index;
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
    uint32_t CompileConstant(const Backend::IL::Constant* constant) {
        switch (constant->type->kind) {
            default:
                ASSERT(false, "Unsupported constant type for recompilation");
                return ~0;
            case Backend::IL::TypeKind::Bool:
                return CompileConstant(static_cast<const Backend::IL::BoolConstant*>(constant));
            case Backend::IL::TypeKind::Int:
                return CompileConstant(static_cast<const Backend::IL::IntConstant*>(constant));
            case Backend::IL::TypeKind::FP:
                return CompileConstant(static_cast<const Backend::IL::FPConstant*>(constant));
        }
    }

    /// Compile a given constant
    uint32_t CompileConstant(const Backend::IL::BoolConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = LLVMBitStreamWriter::EncodeSigned(constant->value);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint32_t CompileConstant(const Backend::IL::IntConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = LLVMBitStreamWriter::EncodeSigned(constant->value);
        return Emit(constant, record);
    }

    /// Compile a given constant
    uint32_t CompileConstant(const Backend::IL::FPConstant* constant) {
        LLVMRecord record(LLVMConstantRecord::Float);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.OpBitWrite(0, constant->value);
        return Emit(constant, record);
    }

    /// Emit a constant and its respective records
    /// \param constant constant to be emitted
    /// \param record constant record
    /// \return DXIL id
    uint32_t Emit(const Backend::IL::Constant* constant, LLVMRecord& record) {
        // Insert type record prior
        LLVMRecord setTypeRecord(LLVMConstantRecord::SetType);
        setTypeRecord.opCount = 1;
        setTypeRecord.ops = recordAllocator.AllocateArray<uint64_t>(1);
        setTypeRecord.ops[0] = typeMap.GetType(constant->type);

        // Emit into block
        declarationBlock->AddRecord(setTypeRecord);

        // Add mapping
        uint32_t id = constantLookup.size();
        AddConstantMapping(constant, id);

        // Always user record
        record.userRecord = true;

        // Emit into block
        declarationBlock->AddRecord(record);

        // OK
        return id;
    }

private:
    /// IL map
    Backend::IL::ConstantMap& programMap;

    /// Identifier map
    Backend::IL::IdentifierMap& identifierMap;

    /// All constants
    std::vector<const IL::Constant*> constants;

    /// IL type to DXIL type table
    std::vector<uint32_t> constantLookup;

    /// Shared type map
    DXILTypeMap& typeMap;

    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

    /// Declaration block
    LLVMBlock* declarationBlock{nullptr};
};
