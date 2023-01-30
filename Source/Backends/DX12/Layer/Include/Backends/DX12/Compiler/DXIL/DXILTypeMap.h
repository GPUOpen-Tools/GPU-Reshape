#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDRemapper.h>

// Backend
#include <Backend/IL/TypeMap.h>
#include <Backend/IL/IdentifierMap.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <vector>

class DXILTypeMap {
public:
    DXILTypeMap(const Allocators& allocators,
                DXILIDRemapper& remapper,
                Backend::IL::TypeMap& programMap,
                Backend::IL::IdentifierMap& identifierMap) :
                programMap(programMap),
                remapper(remapper),
                identifierMap(identifierMap),
                recordAllocator(allocators) {

    }

    /// Set the number of entries
    void SetEntryCount(uint32_t count) {
        indexLookup.resize(count);
    }

    /// Copy this type map
    /// \param out destination map
    void CopyTo(DXILTypeMap& out) {
        out.indexLookup = indexLookup;
        out.typeLookup = typeLookup;
    }

    /// Add a type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const T* AddType(uint32_t index, const T& decl) {
        // LLVM types are indexed separately from global identifiers, so always allocate
        const T* type = programMap.AddType<T>(identifierMap.AllocID(), decl);
        indexLookup.at(index) = type;
        AddTypeMapping(type, index);
        return type;
    }

    /// Add a type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const T* AddUnsortedType(uint32_t index, const T& decl) {
        // LLVM types are indexed separately from global identifiers, so always allocate
        const T* type = programMap.AddUnsortedType<T>(identifierMap.AllocID(), decl);
        indexLookup.at(index) = type;
        AddTypeMapping(type, index);
        return type;
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    const Backend::IL::Type* GetType(uint32_t index) {
        return indexLookup.at(index);
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    uint32_t GetType(const Backend::IL::Type* type) {
        // Allocate if need be
        if (!HasType(type)) {
            return CompileType(type);
        }

        uint32_t id = typeLookup.at(type->id);
        ASSERT(id != ~0u, "Unallocated type");
        return id;
    }

    /// Compile a named type
    /// \param type given type, must be namable
    /// \param name given name
    const Backend::IL::Type* CompileNamedType(const Backend::IL::Type* type, const char* name) {
        if (HasType(type)) {
            return type;
        }

        // Only certain named types
        switch (type->kind) {
            default:
                ASSERT(false, "Type does not support naming");
                break;
            case Backend::IL::TypeKind::Struct:
                CompileType(static_cast<const Backend::IL::StructType*>(type), name);
                break;
        }

        // OK
        return type;
    }

    /// Add a type mapping from IL to DXIL
    /// \param type IL type
    /// \param index DXIL index
    void AddTypeMapping(const Backend::IL::Type* type, uint32_t index) {
        if (typeLookup.size() <= type->id) {
            typeLookup.resize(type->id + 1, ~0u);
        }

        typeLookup[type->id] = index;
    }

    /// Check if there is an existing type mapping
    /// \param type IL type
    /// \return true if present
    bool HasType(const Backend::IL::Type* type) {
        return type->id < typeLookup.size() && typeLookup[type->id] != ~0u;
    }

    /// Set the declaration block for undeclared DXIL types
    /// \param block destination block
    void SetDeclarationBlock(LLVMBlock* block) {
        declarationBlock = block;
    }

    /// Get the number of entries
    uint32_t GetEntryCount() const {
        return static_cast<uint32_t>(indexLookup.size());
    }

    /// Get the program side map
    Backend::IL::TypeMap& GetProgramMap() const {
        return programMap;
    }

private:
    /// Compile a given type
    /// \param type
    /// \return DXIL id
    uint32_t CompileType(const Backend::IL::Type* type) {
        switch (type->kind) {
            default:
                ASSERT(false, "Unsupported type for recompilation");
                return ~0;
            case Backend::IL::TypeKind::Bool:
                return CompileType(static_cast<const Backend::IL::BoolType*>(type));
            case Backend::IL::TypeKind::Void:
                return CompileType(static_cast<const Backend::IL::VoidType*>(type));
            case Backend::IL::TypeKind::Int:
                return CompileType(static_cast<const Backend::IL::IntType*>(type));
            case Backend::IL::TypeKind::FP:
                return CompileType(static_cast<const Backend::IL::FPType*>(type));
            case Backend::IL::TypeKind::Vector:
                return CompileType(static_cast<const Backend::IL::VectorType*>(type));
            case Backend::IL::TypeKind::Pointer:
                return CompileType(static_cast<const Backend::IL::PointerType*>(type));
            case Backend::IL::TypeKind::Array:
                return CompileType(static_cast<const Backend::IL::ArrayType*>(type));
            case Backend::IL::TypeKind::Function:
                return CompileType(static_cast<const Backend::IL::FunctionType*>(type));
            case Backend::IL::TypeKind::Struct:
                return CompileType(static_cast<const Backend::IL::StructType*>(type), nullptr);
        }
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::BoolType* type) {
        LLVMRecord record(LLVMTypeRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = 1u;
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::VoidType* type) {
        LLVMRecord record(LLVMTypeRecord::Void);
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::IntType* type) {
        // LLVM shared signed and unsigned integer types
        if (!type->signedness) {
            uint32_t signedId = GetType(programMap.FindTypeOrAdd(Backend::IL::IntType {
                .bitWidth = type->bitWidth,
                .signedness = true
            }));

            // add mapping
            AddTypeMapping(type, signedId);

            // Set user mapping directly, types are guaranteed to be allocated linearly at the end of the global block
            // There is no risk of collision
            remapper.SetUserMapping(type->id, signedId);

            // OK
            return signedId;
        }

        LLVMRecord record(LLVMTypeRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = type->bitWidth;
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::FPType* type) {
        LLVMRecord record;
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
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::VectorType* type) {
        LLVMRecord record(LLVMTypeRecord::Vector);
        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.opCount = 2;
        record.ops[0] = type->dimension;
        record.ops[1] = GetType(type->containedType);
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::PointerType* type) {
        LLVMRecord record(LLVMTypeRecord::Pointer);
        record.opCount = 2;

        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.ops[0] = GetType(type->pointee);

        // Translate address space
        switch (type->addressSpace) {
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

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::ArrayType* type) {
        LLVMRecord record(LLVMTypeRecord::Array);
        record.opCount = 2;

        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.ops[0] = type->count;
        record.ops[1] = GetType(type->elementType);

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::FunctionType* type) {
        LLVMRecord record(LLVMTypeRecord::Function);
        record.opCount = 2 + type->parameterTypes.size();

        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);
        record.ops[0] = 0;
        record.ops[1] = GetType(type->returnType);

        for (size_t i = 0; i < type->parameterTypes.size(); i++) {
            record.ops[2 + i] = GetType(type->parameterTypes[i]);
        }

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const Backend::IL::StructType* type, const char* name) {
        // Insert name record if needed
        if (name) {
            LLVMRecord record(LLVMTypeRecord::StructName);
            record.SetUser(false, ~0u, ~0u);
            record.opCount = std::strlen(name);
            record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);

            // Copy name
            for (uint32_t i = 0; i < record.opCount; i++) {
                record.ops[i] = name[i];
            }

            // Emit
            declarationBlock->AddRecord(record);
        }

        // Insert struct record, optionally named
        LLVMRecord record(name ? LLVMTypeRecord::StructNamed : LLVMTypeRecord::StructAnon);
        record.opCount = 1 + type->memberTypes.size();
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);
        record.ops[0] = 0;

        for (size_t i = 0; i < type->memberTypes.size(); i++) {
            record.ops[1 + i] = GetType(type->memberTypes[i]);
        }

        return Emit(type, record);
    }

    /// Emit a type and its respective records
    /// \param type type to be emitted
    /// \param record type record
    /// \return DXIL id
    uint32_t Emit(const Backend::IL::Type* type, LLVMRecord& record) {
        // Add mapping
        uint32_t id = static_cast<uint32_t>(indexLookup.size());
        AddTypeMapping(type, id);
        indexLookup.push_back(type);

        // Always user
        record.SetUser(false, ~0u, type->id);

        // Emit into block
        declarationBlock->AddRecord(record);

        // Set user mapping directly, types are guaranteed to be allocated linearly at the end of the global block
        // There is no risk of collision
        remapper.SetUserMapping(type->id, id);

        // OK
        return id;
    }

private:
    /// IL map
    Backend::IL::TypeMap& programMap;

    /// Remapping
    DXILIDRemapper& remapper;

    /// Identifier map
    Backend::IL::IdentifierMap& identifierMap;

    /// Local lookup table
    std::vector<const Backend::IL::Type*> indexLookup;

    /// IL type to DXIL type table
    std::vector<uint32_t> typeLookup;

    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

    /// Declaration block
    LLVMBlock* declarationBlock{nullptr};
};
