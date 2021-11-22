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
    }

    /// Set the declaration stream for allocations
    void SetDeclarationStream(SpvStream *value) {
        declarationStream = value;
    }

    /// Get the id for a given type
    /// \param type the type to looked up
    /// \return the identifier, may be invalid if not found
    SpvId GetId(const SpvType &type) {
        SpvType *typePtr = FindType(type);
        if (!typePtr) {
            return InvalidSpvId;
        }

        return typePtr->id;
    }

    /// Get the id for a given type, or add it if not found
    /// \param type the type to looked up or added
    /// \return the identifier
    SpvId GetIdOrAdd(const SpvType &type) {
        SpvType *typePtr = FindTypeOrAdd(type);
        ASSERT(typePtr, "Failed to create type");

        return typePtr->id;
    }

    /// Add a type to this map, must be unique
    /// \param type the type to be added
    void AddType(const SpvType &type) {
        ASSERT(type.id != InvalidSpvId, "AddType must have a valid id");
        FindTypeOrAdd(type);
    }

    /// Set a type relation in this map
    /// \param id the id to be associated
    /// \param type the resulting type
    void SetType(SpvId id, const SpvType* type) {
        ASSERT(id != InvalidSpvId, "SetType must have a valid id");
        idMap[id] = type;
    }

    /// Get the type for a given id
    /// \param id the id to be looked up
    /// \return the resulting type, may be nullptr
    const SpvType* GetType(SpvId id) {
        auto it = idMap.find(id);
        if (it == idMap.end()) {
            return nullptr;
        }

        return it->second;
    }

private:
    /// Find a type pointer
    /// \param type the type to be found
    /// \return may be nullptr
    SpvType *FindType(const SpvType &type) {
        switch (type.kind) {
            default:
            ASSERT(false, "Invalid type");
                break;
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
            case SpvTypeKind::Vector:
            case SpvTypeKind::Matrix:
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
    SpvType *FindTypeOrAdd(const SpvType &type) {
        SpvType *ptr{nullptr};
        switch (type.kind) {
            default: {
                ASSERT(false, "Invalid type");
                return nullptr;
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
            case SpvTypeKind::Vector:
            case SpvTypeKind::Matrix:
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

        // Map it
        idMap[type->id] = type;
        return type;
    }

    void EmitSpvType(const SpvIntType* type) {
        auto* spv = declarationStream->Allocate<SpvTypeIntInstruction>();
        spv->result = type->id;
        spv->bitWidth = type->bitWidth;
        spv->signedness = type->signedness;
    }

    void EmitSpvType(const SpvFPType* type) {
        auto* spv = declarationStream->Allocate<SpvTypeFPInstruction>();
        spv->result = type->id;
        spv->bitWidth = type->bitWidth;
    }

private:
    Allocators allocators;

    /// Block allocator for types, types never need to be freed
    LinearBlockAllocator<1024> blockAllocator;

    /// External id counter
    SpvId* counter{nullptr};

    /// External declaration stream for allocations
    SpvStream *declarationStream{nullptr};

    /// Type cache
    std::map<uint8_t, SpvIntType *> sintMap;
    std::map<uint8_t, SpvIntType *> uintMap;
    std::map<uint8_t, SpvFPType *>  fpMap;

    /// Id lookup
    std::map<SpvId, const SpvType*> idMap;
};
