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

// Backend
#include "ID.h"
#include "Type.h"
#include "ConstantKind.h"

namespace IL {
    struct Constant {
        /// Reinterpret this global
        /// \tparam T the target global
        template<typename T>
        T* As() {
            ASSERT(kind == T::kKind, "Invalid cast");
            return static_cast<T*>(this);
        }
        
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
        T* Cast() {
            if (kind != T::kKind) {
                return nullptr;
            }

            return static_cast<T*>(this);
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

        /// Check if this constant is symbolic
        /// i.e. It is non-semantic
        bool IsSymbolic() const {
            return id == InvalidID;
        }

        const IL::Type* type{nullptr};

        IL::ConstantKind kind{IL::ConstantKind::None};

        ID id{::IL::InvalidID};
    };

    struct UnexposedConstant : public Constant {
        using Type = IL::UnexposedType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Unexposed;

        auto SortKey(const Type* _type) const {
            return _type->SortKey();
        }
    };

    struct BoolConstant : public Constant {
        using Type = IL::BoolType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Bool;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        bool value{false};
    };

    struct IntConstant : public Constant {
        using Type = IL::IntType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Int;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        int64_t value{0};
    };

    struct FPConstant : public Constant {
        using Type = IL::FPType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::FP;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), value);
        }

        double value{0};
    };

    struct ArrayConstant : public Constant {
        using Type = IL::ArrayType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Array;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), elements);
        }

        std::vector<const Constant*> elements;
    };

    struct VectorConstant : public Constant {
        using Type = IL::VectorType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Vector;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), elements);
        }

        std::vector<const Constant*> elements;
    };

    struct StructConstant : public Constant {
        using Type = IL::StructType;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Struct;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type->SortKey(), members);
        }

        std::vector<const Constant*> members;
    };

    struct UndefConstant : public Constant {
        using Type = IL::Type;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Undef;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type);
        }
    };

    struct NullConstant : public Constant {
        using Type = IL::Type;

        static constexpr IL::ConstantKind kKind = IL::ConstantKind::Null;

        auto SortKey(const Type* _type) const {
            return std::make_tuple(_type);
        }
    };

    /// Sort key helper
    template<typename T>
    using ConstantSortKey = decltype(std::declval<T>().SortKey(nullptr));
}
