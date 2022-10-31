#pragma once

#include "TypeCommon.h"

namespace Backend::IL {
    inline uint64_t GetPODNonAlignedTypeByteSize(const Type* type) {
        switch (type->kind) {
            case TypeKind::None:
            case TypeKind::Pointer:
            case TypeKind::Unexposed:
            case TypeKind::Texture:
            case TypeKind::Buffer:
            case TypeKind::Sampler:
            case TypeKind::CBuffer:
            case TypeKind::Function: {
                ASSERT(false, "Unexpected type");
                return 0;
            }
            case TypeKind::Void: {
                return 0;
            }
            case TypeKind::Bool: {
                return 1;
            }
            case TypeKind::Int: {
                auto _int = type->As<IntType>();
                ASSERT(_int->bitWidth % 8 == 0, "Invalid bitwidth");
                return _int->bitWidth / 8;
            }
            case TypeKind::FP: {
                auto fp = type->As<FPType>();
                ASSERT(fp->bitWidth % 8 == 0, "Invalid bitwidth");
                return fp->bitWidth / 8;
            }
            case TypeKind::Vector:  {
                auto vector = type->As<VectorType>();
                return GetPODNonAlignedTypeByteSize(vector->containedType) * vector->dimension;
            }
            case TypeKind::Matrix: {
                auto matrix = type->As<MatrixType>();
                return GetPODNonAlignedTypeByteSize(matrix->containedType) * matrix->rows * matrix->columns;
            }
            case TypeKind::Array: {
                auto array = type->As<ArrayType>();
                return GetPODNonAlignedTypeByteSize(array->elementType) * array->count;
            }
            case TypeKind::Struct: {
                auto _struct = type->As<StructType>();

                // Summarize size
                size_t size = 0;
                for (const Type* memberType : _struct->memberTypes) {
                    size += GetPODNonAlignedTypeByteSize(memberType);
                }

                return size;
            }
        }

        return 0.0f;
    }
}
