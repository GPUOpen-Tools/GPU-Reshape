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
                Backend::IL::TypeMap &programMap,
                Backend::IL::IdentifierMap &identifierMap)
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
    const Backend::IL::Type* AddNamedType(uint32_t index, const T& decl, const char* name) {
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
    const Backend::IL::Type* GetType(uint32_t index) {
        return indexLookup.at(index);
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    uint32_t GetType(const Backend::IL::Type* type) {
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
            if constexpr(std::is_same_v<T, Backend::IL::Type>) {
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
    /// Compile a given type, checked for non-canonical properties
    /// \param type given type
    /// \return DXIL id
    uint32_t CompileCanonicalType(const Backend::IL::Type* type) {
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
    const bool IsNonCanonical(const Backend::IL::Type* type) {
        switch (type->kind) {
            default: {
                return false;
            }
            case Backend::IL::TypeKind::Int: {
                return !type->As<Backend::IL::IntType>()->signedness;
            }
            case Backend::IL::TypeKind::Vector: {
                return IsNonCanonical(type->As<Backend::IL::VectorType>()->containedType);
            }
            case Backend::IL::TypeKind::Matrix: {
                return IsNonCanonical(type->As<Backend::IL::MatrixType>()->containedType);
            }
            case Backend::IL::TypeKind::Pointer: {
                return IsNonCanonical(type->As<Backend::IL::PointerType>()->pointee);
            }
            case Backend::IL::TypeKind::Array: {
                return IsNonCanonical(type->As<Backend::IL::ArrayType>()->elementType);
            }
            case Backend::IL::TypeKind::Function: {
                auto* fn = type->As<Backend::IL::FunctionType>();

                for (const Backend::IL::Type* paramType : fn->parameterTypes) {
                    if (IsNonCanonical(paramType)) {
                        return true;
                    }
                }

                return IsNonCanonical(fn->returnType);
            }
            case Backend::IL::TypeKind::Struct: {
                auto *_struct = type->As<Backend::IL::StructType>();

                for (const Backend::IL::Type *memberType: _struct->memberTypes) {
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
    const Backend::IL::Type* GetCanonicalType(const Backend::IL::Type* type) {
        switch (type->kind) {
            default: {
                ASSERT(false, "Invalid type");
                return nullptr;
            }
            case Backend::IL::TypeKind::Bool:
            case Backend::IL::TypeKind::Void:
            case Backend::IL::TypeKind::FP:
            case Backend::IL::TypeKind::Sampler:
            case Backend::IL::TypeKind::CBuffer:
            case Backend::IL::TypeKind::Unexposed: {
                return type;
            }
            case Backend::IL::TypeKind::Texture: {
                auto* _type = type->As<Backend::IL::TextureType>();

                return programMap.FindTypeOrAdd(Backend::IL::TextureType {
                    .sampledType = _type->sampledType ? GetCanonicalType(_type->sampledType) : nullptr,
                    .dimension = _type->dimension,
                    .multisampled = _type->multisampled,
                    .samplerMode = _type->samplerMode,
                    .format = _type->format
                });
            }
            case Backend::IL::TypeKind::Buffer:{
                auto* _type = type->As<Backend::IL::BufferType>();

                return programMap.FindTypeOrAdd(Backend::IL::BufferType {
                    .elementType = _type->elementType ? GetCanonicalType(_type->elementType) : nullptr,
                    .samplerMode = _type->samplerMode,
                    .texelType = _type->texelType
                });
            }
            case Backend::IL::TypeKind::Int: {
                auto* _type = type->As<Backend::IL::IntType>();

                return programMap.FindTypeOrAdd(Backend::IL::IntType {
                    .bitWidth = _type->bitWidth,
                    .signedness = true
                });
            }
            case Backend::IL::TypeKind::Vector: {
                auto* _type = type->As<Backend::IL::VectorType>();

                return programMap.FindTypeOrAdd(Backend::IL::VectorType {
                    .containedType = GetCanonicalType(_type->containedType),
                    .dimension = _type->dimension
                });
            }
            case Backend::IL::TypeKind::Matrix: {
                auto* _type = type->As<Backend::IL::MatrixType>();

                return programMap.FindTypeOrAdd(Backend::IL::MatrixType {
                    .containedType = GetCanonicalType(_type->containedType),
                    .rows = _type->rows,
                    .columns = _type->columns
                });
            }
            case Backend::IL::TypeKind::Pointer: {
                auto* _type = type->As<Backend::IL::PointerType>();

                return programMap.FindTypeOrAdd(Backend::IL::PointerType {
                    .pointee = GetCanonicalType(_type->pointee),
                    .addressSpace = _type->addressSpace
                });
            }
            case Backend::IL::TypeKind::Array: {
                auto* _type = type->As<Backend::IL::ArrayType>();

                return programMap.FindTypeOrAdd(Backend::IL::ArrayType {
                    .elementType = GetCanonicalType(_type->elementType),
                    .count = _type->count
                });
            }
            case Backend::IL::TypeKind::Function: {
                auto* _type = type->As<Backend::IL::FunctionType>();

                Backend::IL::FunctionType declaration;

                declaration.returnType = GetCanonicalType(_type->returnType);

                for (const Backend::IL::Type* paramType : _type->parameterTypes) {
                    declaration.parameterTypes.push_back(GetCanonicalType(paramType));
                }

                return programMap.FindTypeOrAdd(declaration);
            }
            case Backend::IL::TypeKind::Struct: {
                auto *_struct = type->As<Backend::IL::StructType>();

                Backend::IL::StructType declaration;

                for (const Backend::IL::Type *memberType: _struct->memberTypes) {
                    declaration.memberTypes.push_back(GetCanonicalType(memberType));
                }

                return programMap.FindTypeOrAdd(declaration);
            }
        }
    }

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
    uint32_t CompileType(const Backend::IL::FunctionType *type) {
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
    Vector<const Backend::IL::Type*> indexLookup;

    /// Named lookup table
    std::map<std::string, const Backend::IL::Type*> namedLookup;

    /// IL type to DXIL type table
    Vector<uint32_t> typeLookup;

    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

    /// Declaration block
    LLVMBlock* declarationBlock{nullptr};
};
