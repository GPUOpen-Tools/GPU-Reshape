#pragma once

// Layer
#include "Spv.h"
#include "SpvStream.h"
#include "SpvInstruction.h"
#include "SpvTranslation.h"
#include "SpvIdMap.h"

// Backend
#include <Backend/IL/Type.h>
#include <Backend/IL/TypeMap.h>

// Std
#include <map>

struct SpvTypeMap {
    SpvTypeMap(const Allocators& allocators, Backend::IL::TypeMap* programMap) : allocators(allocators), programMap(programMap) {

    }

    /// Create a copy of this type map
    ///   ! Parent lifetime tied to the copy
    /// \return the new type map
    void CopyTo(SpvTypeMap& out) {
        out.spvMap = spvMap;
    }

    /// Set the id counter for allocations
    void SetIdMap(SpvIdMap* value) {
        map = value;
    }

    /// Set the declaration stream for allocations
    void SetDeclarationStream(SpvStream *value) {
        declarationStream = value;
    }

    /// Add a type
    /// \param id the spv identifier
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const Backend::IL::Type* AddType(SpvId id, const T& decl) {
        const Backend::IL::Type* type = programMap->AddType<T>(id, decl);
        AddMapping(id, type);
        return type;
    }

    /// Get a type from a given spv identifier
    /// \param id the identifier
    /// \return nullptr if not found
    const Backend::IL::Type* GetTypeFromId(SpvId id) const {
        auto it = idMap.find(id);
        if (it == idMap.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Get the type for a given id
    /// \param id the id to be looked up
    /// \return the resulting type, may be nullptr
    SpvId GetSpvTypeId(const Backend::IL::Type* type) {
        auto it = spvMap.find(type);
        if (it == spvMap.end()) {
            return EmitSpvType(type);
        }

        return it->second;
    }

private:
    SpvId EmitSpvType(const Backend::IL::Type* type) {
        switch (type->kind) {
            default:
                ASSERT(false, "Invalid type compilation");
                return InvalidSpvId;
            case Backend::IL::TypeKind::Bool:
                return EmitSpvType(static_cast<const Backend::IL::BoolType*>(type));
            case Backend::IL::TypeKind::Void:
                return EmitSpvType(static_cast<const Backend::IL::VoidType*>(type));
            case Backend::IL::TypeKind::Int:
                return EmitSpvType(static_cast<const Backend::IL::IntType*>(type));
            case Backend::IL::TypeKind::FP:
                return EmitSpvType(static_cast<const Backend::IL::FPType*>(type));
            case Backend::IL::TypeKind::Vector:
                return EmitSpvType(static_cast<const Backend::IL::VectorType*>(type));
            case Backend::IL::TypeKind::Matrix:
                return EmitSpvType(static_cast<const Backend::IL::MatrixType*>(type));
            case Backend::IL::TypeKind::Pointer:
                return EmitSpvType(static_cast<const Backend::IL::PointerType*>(type));
            case Backend::IL::TypeKind::Array:
                return EmitSpvType(static_cast<const Backend::IL::ArrayType*>(type));
            case Backend::IL::TypeKind::Texture:
                return EmitSpvType(static_cast<const Backend::IL::TextureType*>(type));
            case Backend::IL::TypeKind::Buffer:
                return EmitSpvType(static_cast<const Backend::IL::BufferType*>(type));
            case Backend::IL::TypeKind::Function:
                return EmitSpvType(static_cast<const Backend::IL::FunctionType*>(type));
        }
    }

    void AddMapping(SpvId id, const Backend::IL::Type* type) {
        spvMap[type] = id;
        idMap[id] = type;
    }

    SpvId EmitSpvType(const Backend::IL::IntType* type) {
        SpvId id = map->Allocate();

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeInt, 4);
        spv[1] = id;
        spv[2] = type->bitWidth;
        spv[3] = type->signedness;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::FPType* type) {
        SpvId id = map->Allocate();

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeFloat, 3);
        spv[1] = id;
        spv[2] = type->bitWidth;

        return id;
    }

    SpvId EmitSpvType(const Backend::IL::VoidType* type) {
        SpvId id = map->Allocate();

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeVoid, 3);
        spv[1] = id;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::BoolType* type) {
        SpvId id = map->Allocate();

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeBool, 2);
        spv[1] = id;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::PointerType* type) {
        SpvId id = map->Allocate();

        SpvId pointee = GetSpvTypeId(type->pointee);

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypePointer, 4);
        spv[1] = id;
        spv[2] = Translate(type->addressSpace);
        spv[3] = pointee;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::VectorType* type) {
        SpvId id = map->Allocate();

        SpvId contained = GetSpvTypeId(type->containedType);

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeVector, 4);
        spv[1] = id;
        spv[2] = contained;
        spv[3] = type->dimension;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::MatrixType* type) {
        SpvId id = map->Allocate();

        SpvId column = GetSpvTypeId(programMap->FindTypeOrAdd(Backend::IL::VectorType {
            .containedType = type->containedType,
            .dimension = type->rows
        }));

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeMatrix, 3);
        spv[1] = id;
        spv[2] = column;
        spv[3] = type->columns;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::ArrayType* type) {
        SpvId id = map->Allocate();

        SpvId element = GetSpvTypeId(type->elementType);

        SpvId dim = GetSpvTypeId(programMap->FindTypeOrAdd(Backend::IL::IntType {
            .bitWidth = 32,
            .signedness = false
        }));

        uint32_t dimId = map->Allocate();

        SpvInstruction& _const = declarationStream->Allocate(SpvOpConstant, 4);
        _const[1] = dim;
        _const[2] = dimId;
        _const[3] = type->count;

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeArray, 4);
        spv[1] = id;
        spv[2] = element;
        spv[3] = dimId;

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::TextureType* type) {
        SpvId id = map->Allocate();

        SpvId sampled = GetSpvTypeId(type->sampledType);

        SpvDim dim;
        bool isArray{false};
        switch (type->dimension) {
            default:
            ASSERT(false, "Invalid dimension");
                return InvalidSpvId;
            case Backend::IL::TextureDimension::Texture1D:
                dim = SpvDim1D;
                break;
            case Backend::IL::TextureDimension::Texture2D:
                dim = SpvDim2D;
                break;
            case Backend::IL::TextureDimension::Texture3D:
                dim = SpvDim3D;
                break;
            case Backend::IL::TextureDimension::Texture1DArray:
                dim = SpvDim1D;
                isArray = true;
                break;
            case Backend::IL::TextureDimension::Texture2DArray:
                dim = SpvDim2D;
                isArray = true;
                break;
            case Backend::IL::TextureDimension::Texture2DCube:
                dim = SpvDimCube;
                break;
        }

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeImage, 9);
        spv[1] = id;
        spv[2] = sampled;
        spv[3] = dim;
        spv[4] = 0;
        spv[5] = isArray;
        spv[6] = type->multisampled;
        spv[7] = 1;
        spv[8] = Translate(type->format);

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::BufferType* type) {
        SpvId id = map->Allocate();

        SpvId element = GetSpvTypeId(type->elementType);

        // Texel buffer?
        if (type->texelType != Backend::IL::Format::None) {
            SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeImage, 9);
            spv[1] = id;
            spv[2] = element;
            spv[3] = SpvDimBuffer;
            spv[4] = 0;
            spv[5] = false;
            spv[6] = 0;
            spv[7] = 2;
            spv[8] = Translate(type->texelType);
        } else {
            ASSERT(false, "Structured buffers not implemented");
            return InvalidSpvId;
        }

        AddMapping(id, type);
        return id;
    }

    SpvId EmitSpvType(const Backend::IL::FunctionType* type) {
        SpvId id = map->Allocate();

        SpvInstruction& spv = declarationStream->Allocate(SpvOpTypeFunction, 3 + type->parameterTypes.size());
        spv[1] = id;
        spv[2] = GetSpvTypeId(type->returnType);

        for (uint64_t i = 0; i < type->parameterTypes.size(); i++) {
            spv[3 + i] = GetSpvTypeId(type->parameterTypes[i]);
        }

        AddMapping(id, type);
        return id;
    }

private:
    Allocators allocators;

    /// Id map
    SpvIdMap* map{nullptr};

    /// External declaration stream for allocations
    SpvStream *declarationStream{nullptr};

    /// IL type map
    Backend::IL::TypeMap* programMap{nullptr};

    /// Bidirectional
    std::map<const Backend::IL::Type*, SpvId> spvMap;
    std::map<SpvId, const Backend::IL::Type*> idMap;
};
