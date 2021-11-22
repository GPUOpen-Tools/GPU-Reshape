#pragma once

// Common
#include <Common/Assert.h>

// Backend
#include "Source.h"
#include "OpCode.h"
#include "LiteralType.h"
#include "ID.h"

namespace IL {
    struct Instruction {
        /// Reinterpret this instruction, asserts on validity
        /// \return the casted instruction
        template<typename T>
        const T* As() const {
            ASSERT(Is<T>(), "Bad instruction cast");
            return static_cast<const T*>(this);
        }

        /// Cast this instruction
        /// \return the casted instruction, nullptr if invalid
        template<typename T>
        const T* Cast() const {
            if (!Is<T>()) {
                return nullptr;
            }

            return static_cast<T*>(this);
        }

        /// Check if this instruction is of type
        template<typename T>
        bool Is() const {
            return T::kOpCode == opCode;
        }

        OpCode opCode;
        ID result;
        Source source;
    };

    struct UnexposedInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Unexposed;

        uint32_t backendOpCode;
    };

    struct LiteralInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Literal;

        LiteralType type;
        uint8_t bitWidth;
        bool signedness;

        union {
            double  fp;
            int64_t integral;
        } value;
    };

    struct LoadInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Load;

        ID address;
    };

    struct StoreInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Store;

        ID address;
        ID value;
    };

    struct AddInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Add;

        ID lhs;
        ID rhs;
    };

    struct StoreBufferInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreBuffer;

        ID buffer;
        ID index;
        ID value;
    };

    struct LoadTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LoadTexture;

        ID texture;
        ID index;
    };

    /// Get the size of an instruction
    /// \param op
    /// \return the byte size
    inline uint64_t GetSize(OpCode op) {
        switch (op) {
            default:
                ASSERT(false, "Missing instruction size mapping");
                return 0;
            case OpCode::None:
                return 0;
            case OpCode::Unexposed:
                return sizeof(UnexposedInstruction);
            case OpCode::Load:
                return sizeof(LoadInstruction);
            case OpCode::Store:
                return sizeof(StoreInstruction);
            case OpCode::LoadTexture:
                return sizeof(LoadTextureInstruction);
            case OpCode::StoreBuffer:
                return sizeof(StoreBufferInstruction);
            case OpCode::Add:
                return sizeof(AddInstruction);
            case OpCode::Literal:
                return sizeof(LiteralInstruction);
        }
    }
}
