#pragma once

// Common
#include <Common/Assert.h>

// Backend
#include <Backend/ShaderExport.h>
#include "Source.h"
#include "OpCode.h"
#include "InlineArray.h"
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

            return static_cast<const T*>(this);
        }

        /// Check if this instruction is of type
        template<typename T>
        bool Is() const {
            return T::kOpCode == opCode;
        }

        /// Is this instruction part of the original source code?
        /// \return true if user instruction
        bool IsUserInstruction() const {
            return source.IsValid();
        }

        OpCode opCode;
        ID result;
        Source source;
    };

    struct UnexposedInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Unexposed;

        uint32_t backendOpCode;
    };

    struct SourceAssociationInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::SourceAssociation;

        uint32_t file;
        uint32_t line;
        uint32_t column;
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

    struct SubInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Sub;

        ID lhs;
        ID rhs;
    };

    struct DivInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Div;

        ID lhs;
        ID rhs;
    };

    struct MulInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Mul;

        ID lhs;
        ID rhs;
    };

    struct StoreBufferInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreBuffer;

        ID buffer;
        ID index;
        ID value;
    };

    struct LoadBufferInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LoadBuffer;

        ID buffer;
        ID index;
    };

    struct ResourceSizeInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::ResourceSize;

        ID resource;
    };

    struct LoadTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LoadTexture;

        ID texture;
        ID index;
    };

    struct StoreTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreTexture;

        ID texture;
        ID index;
        ID texel;
    };

    struct OrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Or;

        ID lhs;
        ID rhs;
    };

    struct AndInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::And;

        ID lhs;
        ID rhs;
    };

    struct EqualInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Equal;

        ID lhs;
        ID rhs;
    };

    struct NotEqualInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::NotEqual;

        ID lhs;
        ID rhs;
    };

    struct LessThanInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LessThan;

        ID lhs;
        ID rhs;
    };

    struct LessThanEqualInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LessThanEqual;

        ID lhs;
        ID rhs;
    };

    struct GreaterThanInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::GreaterThan;

        ID lhs;
        ID rhs;
    };

    struct GreaterThanEqualInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::GreaterThanEqual;

        ID lhs;
        ID rhs;
    };

    struct BitOrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitOr;

        ID lhs;
        ID rhs;
    };

    struct BitAndInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitAnd;

        ID lhs;
        ID rhs;
    };

    struct BitShiftLeftInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitShiftLeft;

        ID value;
        ID shift;
    };

    struct BitShiftRightInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitShiftRight;

        ID value;
        ID shift;
    };

    struct BranchInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Branch;

        ID branch;
    };

    struct BranchControlFlow {
        ID merge{InvalidID};
        ID _continue{InvalidID};
    };

    struct BranchConditionalInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BranchConditional;

        BranchControlFlow controlFlow;

        ID cond;
        ID pass;
        ID fail;
    };

    struct SwitchCase {
        uint32_t literal{0};
        ID branch;
    };

    struct SwitchInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Switch;

        /// Get size of this instruction
        /// \param caseCount number of cases
        /// \return byte size
        static uint64_t GetSize(uint32_t caseCount) {
            return sizeof(SwitchInstruction) + InlineArray<SwitchCase>::ElementSize(caseCount);
        }

        /// Get size of this instruction
        /// \return byte size
        uint64_t GetSize() const {
            return sizeof(SwitchInstruction) + cases.ElementSize();
        }

        BranchControlFlow controlFlow;

        ID value;
        ID _default;

        InlineArray<SwitchCase> cases;
    };

    struct ExportInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Export;

        ShaderExportID exportID;
        ID value;
    };

    struct AllocaInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Alloca;

        ID type;
    };

    struct AnyInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Any;

        ID value;
    };

    struct AllInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::All;

        ID value;
    };

    /// Get the size of an instruction
    /// \param op
    /// \return the byte size
    inline uint64_t GetSize(const Instruction* instruction) {
        switch (instruction->opCode) {
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
            case OpCode::Sub:
                return sizeof(SubInstruction);
            case OpCode::Div:
                return sizeof(DivInstruction);
            case OpCode::Mul:
                return sizeof(MulInstruction);
            case OpCode::Equal:
                return sizeof(EqualInstruction);
            case OpCode::NotEqual:
                return sizeof(NotEqualInstruction);
            case OpCode::LessThan:
                return sizeof(LessThanInstruction);
            case OpCode::LessThanEqual:
                return sizeof(LessThanEqualInstruction);
            case OpCode::GreaterThan:
                return sizeof(GreaterThanInstruction);
            case OpCode::GreaterThanEqual:
                return sizeof(GreaterThanEqualInstruction);
            case OpCode::Branch:
                return sizeof(BranchInstruction);
            case OpCode::BranchConditional:
                return sizeof(BranchConditionalInstruction);
            case OpCode::BitOr:
                return sizeof(BitOrInstruction);
            case OpCode::BitAnd:
                return sizeof(BitAndInstruction);
            case OpCode::BitShiftLeft:
                return sizeof(BitShiftLeftInstruction);
            case OpCode::BitShiftRight:
                return sizeof(BitShiftRightInstruction);
            case OpCode::Export:
                return sizeof(ExportInstruction);
            case OpCode::Alloca:
                return sizeof(AllocaInstruction);
            case OpCode::StoreTexture:
                return sizeof(StoreBufferInstruction);
            case OpCode::SourceAssociation:
                return sizeof(SourceAssociationInstruction);
            case OpCode::Any:
                return sizeof(AnyInstruction);
            case OpCode::All:
                return sizeof(AllInstruction);
            case OpCode::Or:
                return sizeof(OrInstruction);
            case OpCode::And:
                return sizeof(AndInstruction);
            case OpCode::LoadBuffer:
                return sizeof(LoadBufferInstruction);
            case OpCode::ResourceSize:
                return sizeof(ResourceSizeInstruction);
            case OpCode::Switch:
                return static_cast<const SwitchInstruction*>(instruction)->GetSize();
        }
    }
}
