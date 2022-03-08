#pragma once

#include "TypeKind.h"
#include "AddressSpace.h"
#include "TextureDimension.h"
#include "Format.h"
#include "ResourceSamplerMode.h"

#include <Common/Assert.h>

#include <tuple>

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

        TypeKind kind{TypeKind::None};
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

        ResourceSamplerMode samplerMode{ResourceSamplerMode::RuntimeOnly};

        Format format{Format::R32UInt};
    };

    struct BufferType : public Type {
        static constexpr TypeKind kKind = TypeKind::Buffer;

        auto SortKey() const {
            return std::make_tuple(elementType, samplerMode, texelType);
        }

        const Type* elementType{nullptr};

        ResourceSamplerMode samplerMode{ResourceSamplerMode::RuntimeOnly};

        Format texelType{Format::None};
    };

    struct FunctionType : public Type {
        static constexpr TypeKind kKind = TypeKind::Function;

        auto SortKey() const {
            return std::make_tuple(returnType, parameterTypes);
        }

        const Type* returnType{nullptr};

        std::vector<const Type*> parameterTypes;
    };

    /// Sort key helper
    template<typename T>
    using SortKey = decltype(std::declval<T>().SortKey());
}
