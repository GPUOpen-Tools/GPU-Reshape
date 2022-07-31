#pragma once

// Backend
#include "ID.h"
#include "Type.h"
#include "ConstantKind.h"

namespace IL {
    struct Constant {
        /// Reinterpret this global
        /// \tparam T the target global
        template<typename T>
        const T* As() const {
            ASSERT(kind == T::kKind, "Invalid cast");
            return static_cast<const T*>(this);
        }

        /// Cast this global
        /// \tparam T the target global
        /// \return nullptr is invalid cast
        template<typename T>
        const T* Cast() const {
            if (kind != T::kKind) {
                return nullptr;
            }

            return static_cast<const T*>(this);
        }

        /// Check if this type is of global
        /// \tparam T the target global
        template<typename T>
        bool Is() const {
            return kind == T::kKind;
        }

        const Backend::IL::Type* type{nullptr};

        Backend::IL::ConstantKind kind{Backend::IL::ConstantKind::None};

        ::IL::ID id{::IL::InvalidID};
    };

    struct UnexposedConstant : public Constant {
        using Type = Backend::IL::UnexposedType;

        static constexpr Backend::IL::ConstantKind kKind = Backend::IL::ConstantKind::Unexposed;

        auto SortKey(const Type* _type) const {
            return _type->SortKey();
        }
    };

    struct BoolConstant : public Constant {
        using Type = Backend::IL::BoolType;

        static constexpr Backend::IL::ConstantKind kKind = Backend::IL::ConstantKind::Bool;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        bool value{false};
    };

    struct IntConstant : public Constant {
        using Type = Backend::IL::IntType;

        static constexpr Backend::IL::ConstantKind kKind = Backend::IL::ConstantKind::Int;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        int64_t value{0};
    };

    struct FPConstant : public Constant {
        using Type = Backend::IL::FPType;

        static constexpr Backend::IL::ConstantKind kKind = Backend::IL::ConstantKind::FP;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        double value{0};
    };

    struct UndefConstant : public Constant {
        using Type = Backend::IL::Type;

        static constexpr Backend::IL::ConstantKind kKind = Backend::IL::ConstantKind::Undef;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type);
        }
    };

    /// Sort key helper
    template<typename T>
    using ConstantSortKey = decltype(std::declval<T>().SortKey(nullptr));
}
