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

// Backend
#include "TypeKind.h"
#include "ID.h"
#include "Source.h"
#include "AddressSpace.h"
#include "TextureDimension.h"
#include "Format.h"
#include "ResourceSamplerMode.h"

// Common
#include <Common/Assert.h>

// Std
#include <tuple>
#include <vector>

namespace Backend::IL {
    struct Type {
        /// Reinterpret this type
        /// \tparam T the target type
        template<typename T>
        const T* As() const {
            ASSERT(kind == T::kKind, "Invalid cast");
            return static_cast<const T*>(this);
        }

        /// Cast this type
        /// \tparam T the target type
        /// \return nullptr is invalid cast
        template<typename T>
        const T* Cast() const {
            if (kind != T::kKind) {
                return nullptr;
            }

            return static_cast<const T*>(this);
        }

        /// Check if this type is of type
        /// \tparam T the target type
        template<typename T>
        bool Is() const {
            return kind == T::kKind;
        }

        TypeKind kind{TypeKind::None};
        ::IL::ID id{::IL::InvalidID};
        uint32_t sourceOffset{::IL::InvalidOffset};
    };

    struct UnexposedType : public Type {
        static constexpr TypeKind kKind = TypeKind::Unexposed;

        auto SortKey() const {
            return std::make_tuple();
        }
    };

    struct BoolType : public Type {
        static constexpr TypeKind kKind = TypeKind::Bool;

        auto SortKey() const {
            return std::make_tuple();
        }
    };

    struct VoidType : public Type {
        static constexpr TypeKind kKind = TypeKind::Void;

        auto SortKey() const {
            return std::make_tuple();
        }
    };

    struct IntType : public Type {
        static constexpr TypeKind kKind = TypeKind::Int;

        auto SortKey() const {
            return std::make_tuple(bitWidth, signedness);
        }

        uint8_t bitWidth{32};

        bool signedness{false};
    };

    struct FPType : public Type {
        static constexpr TypeKind kKind = TypeKind::FP;

        auto SortKey() const {
            return std::make_tuple(bitWidth);
        }

        uint8_t bitWidth{32};
    };

    struct VectorType : public Type {
        static constexpr TypeKind kKind = TypeKind::Vector;

        auto SortKey() const {
            return std::make_tuple(containedType, dimension);
        }

        const Type* containedType{nullptr};

        uint8_t dimension{1};
    };

    struct MatrixType : public Type {
        static constexpr TypeKind kKind = TypeKind::Matrix;

        auto SortKey() const {
            return std::make_tuple(containedType, rows, columns);
        }

        const Type* containedType{nullptr};

        uint8_t rows{1};

        uint8_t columns{1};
    };

    struct PointerType : public Type {
        static constexpr TypeKind kKind = TypeKind::Pointer;

        auto SortKey() const {
            return std::make_tuple(pointee, addressSpace);
        }

        const Type* pointee{nullptr};

        AddressSpace addressSpace{AddressSpace::Function};
    };

    struct ArrayType : public Type {
        static constexpr TypeKind kKind = TypeKind::Array;

        auto SortKey() const {
            return std::make_tuple(elementType, count);
        }

        const Type* elementType{nullptr};

        uint32_t count{0};
    };

    struct TextureType : public Type {
        static constexpr TypeKind kKind = TypeKind::Texture;

        auto SortKey() const {
            return std::make_tuple(sampledType, dimension, multisampled, samplerMode, format);
        }

        const Type* sampledType{nullptr};

        TextureDimension dimension{TextureDimension::Texture1D};

        bool multisampled{false};

        ResourceSamplerMode samplerMode{ResourceSamplerMode::Compatible};

        Format format{Format::R32UInt};
    };

    struct BufferType : public Type {
        static constexpr TypeKind kKind = TypeKind::Buffer;

        auto SortKey() const {
            return std::make_tuple(elementType, samplerMode, texelType);
        }

        const Type* elementType{nullptr};

        ResourceSamplerMode samplerMode{ResourceSamplerMode::Compatible};

        Format texelType{Format::None};
    };

    struct SamplerType : public Type {
        static constexpr TypeKind kKind = TypeKind::Sampler;

        auto SortKey() const {
            return std::make_tuple(0);
        }
    };

    struct CBufferType : public Type {
        static constexpr TypeKind kKind = TypeKind::CBuffer;

        auto SortKey() const {
            return std::make_tuple(0);
        }
    };

    struct FunctionType : public Type {
        static constexpr TypeKind kKind = TypeKind::Function;

        auto SortKey() const {
            return std::make_tuple(returnType, parameterTypes);
        }

        const Type* returnType{nullptr};

        // TODO: Stack fallback
        std::vector<const Type*> parameterTypes;
    };

    struct StructType : public Type {
        static constexpr TypeKind kKind = TypeKind::Struct;

        auto SortKey() const {
            return std::make_tuple(memberTypes);
        }

        // TODO: Stack fallback
        std::vector<const Type*> memberTypes;
    };

    /// Sort key helper
    template<typename T>
    using SortKey = decltype(std::declval<T>().SortKey());
}
