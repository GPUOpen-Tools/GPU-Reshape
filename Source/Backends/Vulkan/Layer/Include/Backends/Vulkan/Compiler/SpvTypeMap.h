#pragma once

// Layer
#include "Spv.h"
#include "SpvStream.h"
#include "SpvInstruction.h"
#include "SpvType.h"

// Common
#include <Common/LinearBlockAllocator.h>

// Std
#include <map>

/// Type map, provides unique type identifiers to comply with the SPIRV specification
struct SpvTypeMap {
    SpvTypeMap(const Allocators& allocators) : allocators(allocators), blockAllocator(allocators) {

    }

    /// Create a copy of this type map
    ///   ! Parent lifetime tied to the copy
    /// \return the new type map
    SpvTypeMap* Copy() {
        auto* copy = new (allocators) SpvTypeMap(allocators);

        // Copy the maps
        copy->idMap = idMap;
        copy->sintMap = sintMap;
        copy->uintMap = uintMap;
        copy->fpMap = fpMap;

        return copy;
    }

    /// Set the id counter for allocations
    void SetIdCounter(SpvId* value) {
        counter = value;
        SetIdBound(*value + 1);
    }

    void SetIdBound(uint32_t bound) {
        idMap.resize(bound + 1);
    }

    /// Set the declaration stream for allocations
    void SetDeclarationStream(SpvStream *value) {
        declarationStream = value;
    }

    /// Get the id for a given type
    /// \param type the type to looked up
    /// \return the identifier, may be invalid if not found
    const SpvType* FindType(const SpvType &type) {
        return FindSpvType(type);
    }

    /// Get the id for a given type, or add it if not found
    /// \param type the type to looked up or added
    /// \return the identifier
    const SpvType* FindTypeOrAdd(const SpvType &type) {
        return FindSpvTypeOrAdd(type);
    }

    /// Add a type to this map, must be unique
    /// \param type the type to be added
    void AddType(const SpvType &type) {
        ASSERT(type.id != InvalidSpvId, "AddType must have a valid id");
        FindSpvTypeOrAdd(type);
    }

    /// Set a type relation in this map
    /// \param id the id to be associated
    /// \param type the resulting type
    void SetType(SpvId id, const SpvType* type) {
        ASSERT(id != InvalidSpvId, "SetType must have a valid id");
        idMap.at(id) = type;
    }

    /// Get the type for a given id
    /// \param id the id to be looked up
    /// \return the resulting type, may be nullptr
    const SpvType* GetType(SpvId id) {
        const SpvType* type = idMap.at(id);
        if (!type) {
            auto* mut = blockAllocator.Allocate<SpvType>(SpvTypeKind::Unexposed);
            mut->id = id;
            type = mut;
        }

        return type;
    }

private:
    /// Find a type pointer
    /// \param type the type to be found
    /// \return may be nullptr
    SpvType *FindSpvType(const SpvType &type) {
        switch (type.kind) {
            default:
            ASSERT(false, "Invalid type");
                break;
            case SpvTypeKind::Bool: {
                return boolType;
            }
            case SpvTypeKind::Void: {
                return voidType;
            }
            case SpvTypeKind::Pointer: {
                auto&& decl = static_cast<const SpvPointerType &>(type);

                if (auto it = ptrMap.find(decl.pointee->id); it != ptrMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Int: {
                auto&& decl = static_cast<const SpvIntType &>(type);

                auto&& map = decl.signedness ? sintMap : uintMap;
                if (auto it = map.find(decl.bitWidth); it != map.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::FP: {
                auto&& decl = static_cast<const SpvFPType &>(type);

                if (auto it = fpMap.find(decl.bitWidth); it != fpMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Vector: {
                auto&& decl = static_cast<const SpvVectorType &>(type);

                VectorizedBucket& bucket = vectorizedMap[decl.containedType->id];

                if (auto it = bucket.vectorMap.find(decl.dimension); it != bucket.vectorMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Matrix: {
                auto&& decl = static_cast<const SpvMatrixType &>(type);

                VectorizedBucket& bucket = vectorizedMap[decl.containedType->id];

                if (auto it = bucket.matrixMap.find(std::make_pair(decl.columns, decl.rows)); it != bucket.matrixMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Array: {
                auto&& decl = static_cast<const SpvArrayType &>(type);

                if (auto it = arrayMap.find(decl.SortKey()); it != arrayMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Image: {
                auto&& decl = static_cast<const SpvImageType &>(type);

                if (auto it = imageMap.find(decl.SortKey()); it != imageMap.end()) {
                    return it->second;
                }
                break;
            }
            case SpvTypeKind::Compound:
                ASSERT(false, "Not implemented");
                break;
        }

        // No match
        return nullptr;
    }

    /// Find a type pointer or allocate a new one
    /// \param type the type to be found or allocated
    /// \return the resulting type
    SpvType *FindSpvTypeOrAdd(const SpvType &type) {
        SpvType *ptr{nullptr};
        switch (type.kind) {
            default: {
                ASSERT(false, "Invalid type");
                return nullptr;
            }
            case SpvTypeKind::Bool: {
                auto&& decl = static_cast<const SpvBoolType &>(type);

                if (!boolType) {
                    boolType = AllocateType<SpvBoolType>(decl);
                }

                return boolType;
            }
            case SpvTypeKind::Void: {
                auto&& decl = static_cast<const SpvVoidType &>(type);

                if (!voidType) {
                    voidType = AllocateType<SpvVoidType>(decl);
                }

                return voidType;
            }
            case SpvTypeKind::Pointer: {
                auto&& decl = static_cast<const SpvPointerType &>(type);

                auto &typePtr = ptrMap[decl.pointee->id];
                if (!typePtr) {
                    typePtr = AllocateType<SpvPointerType>(decl);
                }

                return typePtr;
            }
            case SpvTypeKind::Int: {
                auto&& decl = static_cast<const SpvIntType &>(type);

                auto&& map = decl.signedness ? sintMap : uintMap;

                auto &typePtr = map[decl.bitWidth];
                if (!typePtr) {
                    typePtr = AllocateType<SpvIntType>(decl);
                }

                return typePtr;
            }
            case SpvTypeKind::FP: {
                auto&& decl = static_cast<const SpvFPType &>(type);

                auto &typePtr = fpMap[decl.bitWidth];
                if (!typePtr) {
                    typePtr = AllocateType<SpvFPType>(decl);
                }

                return typePtr;
            }
            case SpvTypeKind::Vector: {
                auto&& decl = static_cast<const SpvVectorType &>(type);

                VectorizedBucket& bucket = vectorizedMap[decl.containedType->id];

                auto &typePtr = bucket.vectorMap[decl.dimension];
                if (!typePtr) {
                    typePtr = AllocateType<SpvVectorType>(decl);
                }

                return typePtr;
            }
            case SpvTypeKind::Matrix: {
                auto&& decl = static_cast<const SpvMatrixType &>(type);

                VectorizedBucket& bucket = vectorizedMap[decl.containedType->id];

                auto &typePtr = bucket.matrixMap[std::make_pair(decl.columns, decl.rows)];
                if (!typePtr) {
                    typePtr = AllocateType<SpvMatrixType>(decl);
                }

                return typePtr;
            }
            case SpvTypeKind::Array: {
                auto&& decl = static_cast<const SpvArrayType&>(type);

                auto &typeImage = arrayMap[decl.SortKey()];
                if (!typeImage) {
                    typeImage = AllocateType<SpvArrayType>(decl);
                }

                return typeImage;
            }
            case SpvTypeKind::Image: {
                auto&& decl = static_cast<const SpvImageType&>(type);

                auto &typeImage = imageMap[decl.SortKey()];
                if (!typeImage) {
                    typeImage = AllocateType<SpvImageType>(decl);
                }

                return typeImage;
            }
            case SpvTypeKind::Compound: {
                ASSERT(false, "Not implemented");
                return nullptr;
            }
        }
    }

    /// Allocate a new type
    /// \tparam T the type cxx type
    /// \param decl the declaration specifier
    /// \return the allocated type
    template<typename T>
    T* AllocateType(const T& decl) {
        auto* type = blockAllocator.Allocate<T>(decl);

        // If the id is invalid, it's a new type
        if (type->id == InvalidSpvId) {
            // Allocate new id
            ASSERT(counter, "Allocating type with no id counter");
            type->id = (*counter)++;

            // Insert into declaration stream
            ASSERT(declarationStream, "Allocating type with no declaration stream");
            EmitSpvType(type);
        }

        if (idMap.size() <= type->id) {
            idMap.resize(type->id + 1);
        }

        // Map it
        idMap.at(type->id) = type;
        return type;
    }

    void EmitSpvType(const SpvIntType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeInt, 4);
        spv[1] = type->id;
        spv[2] = type->bitWidth;
        spv[3] = type->signedness;
    }

    void EmitSpvType(const SpvFPType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeFloat, 3);
        spv[1] = type->id;
        spv[2] = type->bitWidth;
    }

    void EmitSpvType(const SpvVoidType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeVoid, 3);
        spv[1] = type->id;
    }

    void EmitSpvType(const SpvBoolType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeBool, 2);
        spv[1] = type->id;
    }

    void EmitSpvType(const SpvPointerType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypePointer, 4);
        spv[1] = type->id;
        spv[2] = type->storageClass;
        spv[3] = type->pointee->id;
    }

    void EmitSpvType(const SpvVectorType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeVector, 3);
        spv[1] = type->id;
        spv[2] = type->containedType->id;
        spv[3] = type->dimension;
    }

    void EmitSpvType(const SpvMatrixType* type) {
        SpvVectorType columnDecl;
        columnDecl.containedType = type->containedType;
        columnDecl.id = type->rows;
        auto columnType = FindSpvTypeOrAdd(columnDecl);

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeMatrix, 3);
        spv[1] = type->id;
        spv[2] = columnType->id;
        spv[3] = type->columns;
    }

    void EmitSpvType(const SpvArrayType* type) {
        SpvIntType typeDecl;
        typeDecl.signedness = false;
        typeDecl.bitWidth = 32;
        const SpvType* dimType = FindSpvTypeOrAdd(typeDecl);

        uint32_t dimId = (*counter)++;

        SpvInstruction& _const = declarationStream->Allocate(SpvOpConstant, 4);
        _const[1] = dimType->id;
        _const[2] = dimId;
        _const[3] = type->count;

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeArray, 4);
        spv[1] = type->id;
        spv[2] = type->elementType->id;
        spv[3] = dimId;
    }

    void EmitSpvType(const SpvImageType* type) {
        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeImage, 9);
        spv[1] = type->id;
        spv[2] = type->sampledType->id;
        spv[3] = type->dimension;
        spv[4] = type->depth;
        spv[5] = type->arrayed;
        spv[6] = type->multisampled;
        spv[7] = type->sampled;
        spv[8] = type->format;
    }

private:
    Allocators allocators;

    /// Block allocator for types, types never need to be freed
    LinearBlockAllocator<1024> blockAllocator;

    /// External id counter
    SpvId* counter{nullptr};

    /// External declaration stream for allocations
    SpvStream *declarationStream{nullptr};

    /// Inbuilt
    SpvVoidType* voidType{nullptr};
    SpvBoolType* boolType{nullptr};

    /// Vectorized types
    struct VectorizedBucket {
        std::map<uint8_t, SpvVectorType *>                     vectorMap;
        std::map<std::pair<uint8_t, uint8_t>, SpvMatrixType *> matrixMap;
    };

    /// Type cache
    std::map<uint8_t, SpvIntType *>     sintMap;
    std::map<uint8_t, SpvIntType *>     uintMap;
    std::map<uint8_t, SpvFPType *>      fpMap;
    std::map<SpvId,   SpvPointerType *> ptrMap;
    std::map<SpvId,   VectorizedBucket> vectorizedMap;

    /// Complex type cache
    std::map<SpvArraySortKeyType, SpvArrayType*> arrayMap;
    std::map<SpvImageSortKeyType, SpvImageType*> imageMap;

    /// Id lookup
    std::vector<const SpvType*> idMap;
};
