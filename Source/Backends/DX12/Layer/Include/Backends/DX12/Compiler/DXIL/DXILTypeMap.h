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
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDRemapper.h>

// Backend
#include <Backend/IL/TypeMap.h>
#include <Backend/IL/IdentifierMap.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>
#include <Common/Alloca.h>

// Std
#include <vector>
#include <string>
#include <map>

class DXILTypeMap {
public:
    DXILTypeMap(const Allocators &allocators,
                DXILIDRemapper &remapper,
                IL::TypeMap &programMap,
                IL::IdentifierMap &identifierMap)
        : programMap(programMap),
          remapper(remapper),
          identifierMap(identifierMap),
          indexLookup(allocators.Tag(kAllocModuleDXILTypeMap)),
          typeLookup(allocators.Tag(kAllocModuleDXILTypeMap)),
          recordAllocator(allocators.Tag(kAllocModuleDXILTypeMap)) {

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
        out.namedLookup = namedLookup;
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

    /// Add a named type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \param name given name
    template<typename T>
    const IL::Type* AddNamedType(uint32_t index, const T& decl, const char* name) {
        ASSERT(!namedLookup.contains(name), "Duplicate named type");

        // LLVM types are indexed separately from global identifiers, so always allocate
        const T* type = programMap.AddUnsortedType<T>(identifierMap.AllocID(), decl);
        indexLookup.at(index) = type;
        AddTypeMapping(type, index);

        // Named mapping
        namedLookup[name] = type;

        // OK
        return type;
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    const IL::Type* GetType(uint32_t index) {
        return indexLookup.at(index);
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    uint32_t GetType(const IL::Type* type) {
        // Allocate if need be
        if (!HasType(type)) {
            return CompileCanonicalType(type);
        }

        uint32_t id = typeLookup.at(type->id);
        ASSERT(id != ~0u, "Unallocated type");
        return id;
    }

    /// Compile a named type
    /// \param type given type, must be namable
    /// \param name given name
    template<typename T>
    const T* CompileNamedType(const T* type, const char* name) {
        auto&& it = namedLookup.find(name);

        // Named types use name as key
        // TODO: Validate fetched type against requested
        if (it != namedLookup.end()) {
            if constexpr(std::is_same_v<T, IL::Type>) {
                return it->second;
            } else {
                return it->second->As<T>();
            }
        }

        // Only certain named types
        switch (type->kind) {
            default:
                ASSERT(false, "Type does not support naming");
                break;
            case IL::TypeKind::Struct:
                CompileType(static_cast<const IL::StructType*>(type), name);
                break;
        }

        // OK
        return type;
    }

    /// Find or compile a named type declaration
    /// \param typeDecl given type declaration, must be namable
    /// \param name given name
    template<typename T>
    const T* CompileOrFindNamedType(const T& typeDecl, const char* name) {
        auto&& it = namedLookup.find(name);

        // Named types use name as key
        // TODO: Validate fetched type against requested
        if (it != namedLookup.end()) {
            if constexpr(std::is_same_v<T, IL::Type>) {
                return it->second;
            } else {
                return it->second->As<T>();
            }
        }

        // Not found, allocate the type
        const T* type = programMap.AddUnsortedType(identifierMap.AllocID(), typeDecl);

        // Only certain named types
        switch (type->kind) {
            default:
                ASSERT(false, "Type does not support naming");
                break;
            case IL::TypeKind::Struct:
                CompileType(static_cast<const IL::StructType*>(type), name);
                break;
        }

        // OK
        return type;
    }

    /// Add a type mapping from IL to DXIL
    /// \param type IL type
    /// \param index DXIL index
    void AddTypeMapping(const IL::Type* type, uint32_t index) {
        if (typeLookup.size() <= type->id) {
            typeLookup.resize(type->id + 1, ~0u);
        }

        typeLookup[type->id] = index;
    }

    /// Check if there is an existing type mapping
    /// \param type IL type
    /// \return true if present
    bool HasType(const IL::Type* type) {
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
    IL::TypeMap& GetProgramMap() const {
        return programMap;
    }

private:
    /// Compile a given type, checked for non-canonical properties
    /// \param type given type
    /// \return DXIL id
    uint32_t CompileCanonicalType(const IL::Type* type) {
        // Check for non-canonical properties first, faster than out-right creating it
        if (IsNonCanonical(type)) {
            // Filter constraints on type
            type = GetCanonicalType(type);

            // Already exists?
            if (HasType(type)) {
                uint32_t id = typeLookup.at(type->id);
                ASSERT(id != ~0u, "Unallocated type");
                return id;
            }
        }

        // Compile canonical type
        return CompileType(type);
    }

    /// Check if a type has non-canonical proeprties
    /// \param type given type
    /// \return true if non-canonical
    const bool IsNonCanonical(const IL::Type* type) {
        switch (type->kind) {
            default: {
                return false;
            }
            case IL::TypeKind::Int: {
                return !type->As<IL::IntType>()->signedness;
            }
            case IL::TypeKind::Vector: {
                return IsNonCanonical(type->As<IL::VectorType>()->containedType);
            }
            case IL::TypeKind::Matrix: {
                return IsNonCanonical(type->As<IL::MatrixType>()->containedType);
            }
            case IL::TypeKind::Pointer: {
                return IsNonCanonical(type->As<IL::PointerType>()->pointee);
            }
            case IL::TypeKind::Array: {
                return IsNonCanonical(type->As<IL::ArrayType>()->elementType);
            }
            case IL::TypeKind::Function: {
                auto* fn = type->As<IL::FunctionType>();

                for (const IL::Type* paramType : fn->parameterTypes) {
                    if (IsNonCanonical(paramType)) {
                        return true;
                    }
                }

                return IsNonCanonical(fn->returnType);
            }
            case IL::TypeKind::Struct: {
                auto *_struct = type->As<IL::StructType>();

                for (const IL::Type *memberType: _struct->memberTypes) {
                    if (IsNonCanonical(memberType)) {
                        return true;
                    }
                }

                return false;
            }
        }
    }

    /// Get the canonical type
    /// \param type source type
    /// \return canonical type
    const IL::Type* GetCanonicalType(const IL::Type* type) {
        switch (type->kind) {
            default: {
                ASSERT(false, "Invalid type");
                return nullptr;
            }
            case IL::TypeKind::Bool:
            case IL::TypeKind::Void:
            case IL::TypeKind::FP:
            case IL::TypeKind::Sampler:
            case IL::TypeKind::CBuffer:
            case IL::TypeKind::Unexposed: {
                return type;
            }
            case IL::TypeKind::Texture: {
                auto* _type = type->As<IL::TextureType>();

                return programMap.FindTypeOrAdd(IL::TextureType {
                    .sampledType = _type->sampledType ? GetCanonicalType(_type->sampledType) : nullptr,
                    .dimension = _type->dimension,
                    .multisampled = _type->multisampled,
                    .samplerMode = _type->samplerMode,
                    .format = _type->format
                });
            }
            case IL::TypeKind::Buffer:{
                auto* _type = type->As<IL::BufferType>();

                return programMap.FindTypeOrAdd(IL::BufferType {
                    .elementType = _type->elementType ? GetCanonicalType(_type->elementType) : nullptr,
                    .samplerMode = _type->samplerMode,
                    .texelType = _type->texelType
                });
            }
            case IL::TypeKind::Int: {
                auto* _type = type->As<IL::IntType>();

                return programMap.FindTypeOrAdd(IL::IntType {
                    .bitWidth = _type->bitWidth,
                    .signedness = true
                });
            }
            case IL::TypeKind::Vector: {
                auto* _type = type->As<IL::VectorType>();

                return programMap.FindTypeOrAdd(IL::VectorType {
                    .containedType = GetCanonicalType(_type->containedType),
                    .dimension = _type->dimension
                });
            }
            case IL::TypeKind::Matrix: {
                auto* _type = type->As<IL::MatrixType>();

                return programMap.FindTypeOrAdd(IL::MatrixType {
                    .containedType = GetCanonicalType(_type->containedType),
                    .rows = _type->rows,
                    .columns = _type->columns
                });
            }
            case IL::TypeKind::Pointer: {
                auto* _type = type->As<IL::PointerType>();

                return programMap.FindTypeOrAdd(IL::PointerType {
                    .pointee = GetCanonicalType(_type->pointee),
                    .addressSpace = _type->addressSpace
                });
            }
            case IL::TypeKind::Array: {
                auto* _type = type->As<IL::ArrayType>();

                return programMap.FindTypeOrAdd(IL::ArrayType {
                    .elementType = GetCanonicalType(_type->elementType),
                    .count = _type->count
                });
            }
            case IL::TypeKind::Function: {
                auto* _type = type->As<IL::FunctionType>();

                IL::FunctionType declaration;

                declaration.returnType = GetCanonicalType(_type->returnType);

                for (const IL::Type* paramType : _type->parameterTypes) {
                    declaration.parameterTypes.push_back(GetCanonicalType(paramType));
                }

                return programMap.FindTypeOrAdd(declaration);
            }
            case IL::TypeKind::Struct: {
                auto *_struct = type->As<IL::StructType>();

                IL::StructType declaration;

                for (const IL::Type *memberType: _struct->memberTypes) {
                    declaration.memberTypes.push_back(GetCanonicalType(memberType));
                }

                return programMap.FindTypeOrAdd(declaration);
            }
        }
    }

    /// Compile a given type
    /// \param type
    /// \return DXIL id
    uint32_t CompileType(const IL::Type* type) {
        switch (type->kind) {
            default:
                ASSERT(false, "Unsupported type for recompilation");
                return ~0;
            case IL::TypeKind::Bool:
                return CompileType(static_cast<const IL::BoolType*>(type));
            case IL::TypeKind::Void:
                return CompileType(static_cast<const IL::VoidType*>(type));
            case IL::TypeKind::Int:
                return CompileType(static_cast<const IL::IntType*>(type));
            case IL::TypeKind::FP:
                return CompileType(static_cast<const IL::FPType*>(type));
            case IL::TypeKind::Vector:
                return CompileType(static_cast<const IL::VectorType*>(type));
            case IL::TypeKind::Pointer:
                return CompileType(static_cast<const IL::PointerType*>(type));
            case IL::TypeKind::Array:
                return CompileType(static_cast<const IL::ArrayType*>(type));
            case IL::TypeKind::Function:
                return CompileType(static_cast<const IL::FunctionType*>(type));
            case IL::TypeKind::Struct:
                return CompileType(static_cast<const IL::StructType*>(type), nullptr);
        }
    }

    /// Compile a given type
    uint32_t CompileType(const IL::BoolType* type) {
        LLVMRecord record(LLVMTypeRecord::Integer);
        record.opCount = 1;
        record.ops = recordAllocator.AllocateArray<uint64_t>(1);
        record.ops[0] = 1u;
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::VoidType* type) {
        LLVMRecord record(LLVMTypeRecord::Void);
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::IntType* type) {
        // LLVM shared signed and unsigned integer types
        if (!type->signedness) {
            uint32_t signedId = GetType(programMap.FindTypeOrAdd(IL::IntType {
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
    uint32_t CompileType(const IL::FPType* type) {
        LLVMRecord record;
        switch (type->As<IL::FPType>()->bitWidth) {
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
    uint32_t CompileType(const IL::VectorType* type) {
        LLVMRecord record(LLVMTypeRecord::Vector);
        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.opCount = 2;
        record.ops[0] = type->dimension;
        record.ops[1] = GetType(type->containedType);
        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::PointerType* type) {
        LLVMRecord record(LLVMTypeRecord::Pointer);
        record.opCount = 2;

        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.ops[0] = GetType(type->pointee);

        // Translate address space
        switch (type->addressSpace) {
            default:
            ASSERT(false, "Invalid address space");
                break;
            case IL::AddressSpace::Constant:
                record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Constant);
                break;
            case IL::AddressSpace::Function:
                record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Local);
                break;
            case IL::AddressSpace::Texture:
            case IL::AddressSpace::Buffer:
            case IL::AddressSpace::Resource:
                record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::Device);
                break;
            case IL::AddressSpace::GroupShared:
                record.ops[1] = static_cast<uint64_t>(DXILAddressSpace::GroupShared);
                break;
        }

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::ArrayType* type) {
        LLVMRecord record(LLVMTypeRecord::Array);
        record.opCount = 2;

        record.ops = recordAllocator.AllocateArray<uint64_t>(2);
        record.ops[0] = type->count;
        record.ops[1] = GetType(type->elementType);

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::FunctionType *type) {
        // Get return uid
        uint32_t returnId = GetType(type->returnType);
        
        // Get parameter uids
        auto *parameters = ALLOCA_ARRAY(uint32_t, type->parameterTypes.size());
        for (uint64_t i = 0; i < type->parameterTypes.size(); i++) {
            parameters[i] = GetType(type->parameterTypes[i]);
        }

        // Setup record
        LLVMRecord record(LLVMTypeRecord::Function);
        record.opCount = 2 + type->parameterTypes.size();
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);
        record.ops[0] = 0;
        record.ops[1] = returnId;

        // Copy parameters
        for (size_t i = 0; i < type->parameterTypes.size(); i++) {
            record.ops[2 + i] = parameters[i];
        }

        return Emit(type, record);
    }

    /// Compile a given type
    uint32_t CompileType(const IL::StructType* type, const char* name) {
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

            // Store lookup
            namedLookup[name] = type;
        }

        // Get member uids
        auto *members = ALLOCA_ARRAY(uint32_t, type->memberTypes.size());
        for (uint64_t i = 0; i < type->memberTypes.size(); i++) {
            members[i] = GetType(type->memberTypes[i]);
        }

        // Insert struct record, optionally named
        LLVMRecord record(name ? LLVMTypeRecord::StructNamed : LLVMTypeRecord::StructAnon);
        record.opCount = 1 + type->memberTypes.size();
        record.ops = recordAllocator.AllocateArray<uint64_t>(record.opCount);
        record.ops[0] = 0;

        // Copy members
        for (size_t i = 0; i < type->memberTypes.size(); i++) {
            record.ops[1 + i] = members[i];
        }

        return Emit(type, record);
    }

    /// Emit a type and its respective records
    /// \param type type to be emitted
    /// \param record type record
    /// \return DXIL id
    uint32_t Emit(const IL::Type* type, LLVMRecord& record) {
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
    IL::TypeMap& programMap;

    /// Remapping
    DXILIDRemapper& remapper;

    /// Identifier map
    IL::IdentifierMap& identifierMap;

    /// Local lookup table
    Vector<const IL::Type*> indexLookup;

    /// Named lookup table
    std::map<std::string, const IL::Type*> namedLookup;

    /// IL type to DXIL type table
    Vector<uint32_t> typeLookup;

    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

    /// Declaration block
    LLVMBlock* declarationBlock{nullptr};
};
