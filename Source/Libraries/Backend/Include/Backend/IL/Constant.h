#pragma once

// Backend
#include "ID.h"
#include "Type.h"

namespace IL {
    struct Constant {
        /// Reinterpret this global
        /// \tparam T the target global
        template<typename T>
        const T* As() const {
            ASSERT(type->kind == T::Type::kKind, "Invalid cast");
            return static_cast<const T*>(this);
        }

        /// Cast this global
        /// \tparam T the target global
        /// \return nullptr is invalid cast
        template<typename T>
        const T* Cast() const {
            if (type->kind != T::Type::kKind) {
                return nullptr;
            }

            return static_cast<const T*>(this);
        }

        /// Check if this type is of global
        /// \tparam T the target global
        template<typename T>
        bool Is() const {
            return type->kind == T::Type::kKind;
        }

        const Backend::IL::Type* type{nullptr};
        ::IL::ID id{::IL::InvalidID};
    };

    struct UnexposedConstant : public Constant {
        using Type = Backend::IL::UnexposedType;

        auto SortKey(const Type* _type) const {
            return _type->SortKey();
        }
    };

    struct BoolConstant : public Constant {
        using Type = Backend::IL::BoolType;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        bool value{false};
    };

    struct IntConstant : public Constant {
        using Type = Backend::IL::IntType;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        int64_t value{0};
    };

    struct FPConstant : public Constant {
        using Type = Backend::IL::FPType;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        double value{0};
    };

    /// Sort key helper
    template<typename T>
    using ConstantSortKey = decltype(std::declval<T>().SortKey(nullptr));
}
