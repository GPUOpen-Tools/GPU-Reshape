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

#include "Program.h"
#include "Instruction.h"

namespace IL {
    inline const Type* GetComponentType(const Type* type) {
        switch (type->kind) {
            default:
                return type;
            case TypeKind::Struct:
                ASSERT(false, "Structural types require an index");
                return nullptr;
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
    
    inline uint32_t GetTypeDimension(const Type* type) {
        switch (type->kind) {
            default:
                return 1u;
            case TypeKind::Vector:
                return type->As<VectorType>()->dimension;
            case TypeKind::Matrix:
                return type->As<MatrixType>()->rows * type->As<MatrixType>()->columns;
            case TypeKind::Array:
                return type->As<ArrayType>()->count;
        }
    }

    inline const Type* Splat(Program& program, const Type* scalarType, uint8_t count) {
        return program.GetTypeMap().FindTypeOrAdd(IL::VectorType {
            .containedType = scalarType,
            .dimension = count
        });
    }

    inline const Type* SplatToValue(Program& program, const Type* scalarType, IL::ID value) {
        const Type* valueType = program.GetTypeMap().GetType(value);
        if (!valueType) {
            ASSERT(false, "No type on splat value");
            return nullptr;
        }

        // Splat to target type
        switch (valueType->kind) {
            default:
                ASSERT(false, "Invalid splat target");
                return nullptr;
            case TypeKind::Bool:
            case TypeKind::Int:
            case TypeKind::FP:
                return scalarType;
            case TypeKind::Vector:
                return Splat(program, scalarType, valueType->As<VectorType>()->dimension);
        }
    }

    inline const bool IsComponentType(const Type* type, TypeKind kind) {
        switch (type->kind) {
            default:
                return type->kind == kind;
            case TypeKind::Vector:
                return type->As<VectorType>()->containedType->kind == kind;
            case TypeKind::Matrix:
                return type->As<MatrixType>()->containedType->kind == kind;
        }
    }

    template<typename T>
    inline const bool IsComponentType(const Type* type) {
        return IsComponentType(type, T::kKind);
    }

    inline const Type* GetValueType(const Type* type) {
        switch (type->kind) {
            default:
                return type;
            case TypeKind::Array:
                return type->As<ArrayType>()->elementType;
            case TypeKind::Pointer:
                return type->As<PointerType>()->pointee;
        }
    }

    inline const Type* GetTerminalValueType(const Type* type) {
        for (;;) {
            const Type* next = GetValueType(type);
            
            if (next == type) {
                return type;
            }

            type = next;
        }
    }

    inline const bool IsResourceType(const Type* type) {
        switch (type->kind) {
            default:
                return false;
            case TypeKind::Texture:
                return true;
            case TypeKind::Buffer:
                return true;
        }
    }

    inline const bool IsPointerToResourceType(const Type* type) {
        auto ptr = type->Cast<PointerType>();
        if (!ptr) {
            return false;
        }

        return IsResourceType(ptr->pointee);
    }

    inline bool IsGenericResourceAddressSpace(const PointerType* type) {
        switch (type->addressSpace) {
            default:
                return false;
            case AddressSpace::Texture:
            case AddressSpace::Buffer:
            case AddressSpace::Resource:
            case AddressSpace::Constant:
                return true;
        }
    }

    inline const Type* GetStructuralType(const Type* type, uint32_t index) {
        switch (type->kind) {
            default:
                return GetComponentType(type);
            case TypeKind::Struct: {
                return type->As<StructType>()->memberTypes[index];
            }
        }
    }

    inline const bool IsScalarType(const Type* type) {
        switch (type->kind) {
            default:
                return false;
            case TypeKind::FP:
                return true;
            case TypeKind::Int:
                return true;
            case TypeKind::Bool:
                return true;
        }
    }
}
