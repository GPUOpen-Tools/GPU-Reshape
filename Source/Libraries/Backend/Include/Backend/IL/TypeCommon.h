#pragma once

#include "Program.h"
#include "Instruction.h"

namespace Backend::IL {
    inline const Type* GetComponentType(const Type* type) {
        switch (type->kind) {
            default:
                return type;
            case TypeKind::Vector:
                return type->As<VectorType>()->containedType;
            case TypeKind::Matrix:
                return type->As<MatrixType>()->containedType;
            case TypeKind::Array:
                return type->As<ArrayType>()->elementType;
            case TypeKind::Texture:
                return type->As<TextureType>()->sampledType;
            case TypeKind::Buffer:
                return type->As<BufferType>()->elementType;
        }
    }
}
