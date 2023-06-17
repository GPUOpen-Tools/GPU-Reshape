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
#include "ComponentMask.h"
#include "TextureSampleMode.h"

namespace IL {
    struct Instruction {
        /// Reinterpret this instruction, asserts on validity
        /// \return the casted instruction
        template<typename T>
        T* As() {
            ASSERT(Is<T>(), "Bad instruction cast");
            return static_cast<T*>(this);
        }

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
        T* Cast() {
            if (!Is<T>()) {
                return nullptr;
            }

            return static_cast<T*>(this);
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

        const char* symbol;
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

    struct StoreOutputInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreOutput;

        ID index;
        ID row;
        ID column;
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

    struct RemInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Rem;

        ID lhs;
        ID rhs;
    };

    struct TruncInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Trunc;

        ID value;
    };

    struct StoreBufferInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreBuffer;

        ID buffer;
        ID index;
        ID value;
        ComponentMaskSet mask;
    };

    struct LoadBufferInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LoadBuffer;

        ID buffer;
        ID index;
        ID offset;
    };

    struct ResourceSizeInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::ResourceSize;

        ID resource;
    };

    struct ResourceTokenInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::ResourceToken;

        ID resource;
    };

    struct SampleTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::SampleTexture;

        Backend::IL::TextureSampleMode sampleMode;

        ID texture;
        ID sampler;
        ID coordinate;

        /// Value reference to sampling mode, optional
        ID reference;

        /// Explicit lod, optional
        ID lod;

        /// Lod bias, optional
        ID bias;

        /// Explicit gradients, optional
        ID ddx;
        ID ddy;

        /// Offset, optional
        ID offset;
    };

    struct LoadTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::LoadTexture;

        ID texture;
        ID index;

        /// Offset, optional
        ID offset;

        /// Mip level, optional
        ID mip;
    };

    struct StoreTextureInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::StoreTexture;

        ID texture;
        ID index;
        ID texel;
        ComponentMaskSet mask;
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

    struct IsInfInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::IsInf;

        ID value;
    };

    struct IsNaNInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::IsNaN;

        ID value;
    };

    struct BitOrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitOr;

        ID lhs;
        ID rhs;
    };

    struct BitXOrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitXOr;

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

    struct FloatToIntInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::FloatToInt;

        ID value;
    };

    struct IntToFloatInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::IntToFloat;

        ID value;
    };

    struct BitCastInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::BitCast;

        ID value;
    };

    struct AddressChain {
        ID index;
    };

    struct AddressChainInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AddressChain;

        /// Get size of this instruction
        /// \param chainCount number of chains
        /// \return byte size
        static uint64_t GetSize(uint32_t chainCount) {
            return sizeof(AddressChainInstruction) + InlineArray<AddressChain>::ElementSize(chainCount);
        }

        /// Get size of this instruction
        /// \return byte size
        uint64_t GetSize() const {
            return sizeof(AddressChainInstruction) + chains.ElementSize();
        }

        ID composite;
        InlineArray<AddressChain> chains;
    };

    struct ExtractInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Extract;

        ID composite;
        uint32_t index;
    };

    struct InsertInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Extract;

        ID composite;
        ID value;
    };

    struct SelectInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Select;

        ID condition;
        ID pass;
        ID fail;
    };

    struct BranchControlFlow {
        bool Contains(ID id) const {
            return merge == id || _continue == id;
        }
        
        ID merge{InvalidID};
        ID _continue{InvalidID};
    };

    struct BranchInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Branch;

        BranchControlFlow controlFlow;

        ID branch;
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

    struct PhiValue {
        /// Value within branch to be chosen
        ID value;

        /// Owning branch
        ID branch;
    };

    struct PhiInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Phi;

        /// Get size of this instruction
        /// \param caseCount number of values
        /// \return byte size
        static uint64_t GetSize(uint32_t valueCount) {
            return sizeof(PhiInstruction) + InlineArray<PhiValue>::ElementSize(valueCount);
        }

        /// Get size of this instruction
        /// \return byte size
        uint64_t GetSize() const {
            return sizeof(PhiInstruction) + values.ElementSize();
        }

        InlineArray<PhiValue> values;
    };

    struct ReturnInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Return;

        ID value;
    };

    struct AtomicOrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicOr;

        ID address;
        ID value;
    };

    struct AtomicXOrInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicXOr;

        ID address;
        ID value;
    };

    struct AtomicAndInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicAnd;

        ID address;
        ID value;
    };

    struct AtomicAddInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicAdd;

        ID address;
        ID value;
    };

    struct AtomicMinInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicMin;

        ID address;
        ID value;
    };

    struct AtomicMaxInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicMax;

        ID address;
        ID value;
    };

    struct AtomicExchangeInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicExchange;

        ID address;
        ID value;
    };

    struct AtomicCompareExchangeInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::AtomicCompareExchange;

        ID address;
        ID comparator;
        ID value;
    };

    struct ExportInstruction : public Instruction {
        static constexpr OpCode kOpCode = OpCode::Export;

        ShaderExportID exportID;

        /// Get size of this instruction
        /// \param valueCount number of values
        /// \return byte size
        static uint64_t GetSize(uint32_t valueCount) {
            return sizeof(ExportInstruction) + InlineArray<IL::ID>::ElementSize(valueCount);
        }

        /// Get size of this instruction
        /// \return byte size
        uint64_t GetSize() const {
            return sizeof(ExportInstruction) + values.ElementSize();
        }

        InlineArray<IL::ID> values;
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
            case OpCode::SampleTexture:
                return sizeof(SampleTextureInstruction);
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
            case OpCode::Rem:
                return sizeof(RemInstruction);
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
            case OpCode::BitXOr:
                return sizeof(BitXOrInstruction);
            case OpCode::BitAnd:
                return sizeof(BitAndInstruction);
            case OpCode::BitShiftLeft:
                return sizeof(BitShiftLeftInstruction);
            case OpCode::BitShiftRight:
                return sizeof(BitShiftRightInstruction);
            case OpCode::Export:
                return static_cast<const ExportInstruction*>(instruction)->GetSize();
            case OpCode::Alloca:
                return sizeof(AllocaInstruction);
            case OpCode::StoreTexture:
                return sizeof(StoreBufferInstruction);
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
            case OpCode::ResourceToken:
                return sizeof(ResourceTokenInstruction);
            case OpCode::Switch:
                return static_cast<const SwitchInstruction*>(instruction)->GetSize();
            case OpCode::Phi:
                return static_cast<const PhiInstruction*>(instruction)->GetSize();
            case OpCode::Trunc:
                return sizeof(TruncInstruction);
            case OpCode::Return:
                return sizeof(ReturnInstruction);
            case OpCode::FloatToInt:
                return sizeof(FloatToIntInstruction);
            case OpCode::IntToFloat:
                return sizeof(IntToFloatInstruction);
            case OpCode::BitCast:
                return sizeof(BitCastInstruction);
            case OpCode::AddressChain:
                return static_cast<const AddressChainInstruction*>(instruction)->GetSize();
            case OpCode::Extract:
                return sizeof(ExtractInstruction);
            case OpCode::Insert:
                return sizeof(InsertInstruction);
            case OpCode::Select:
                return sizeof(SelectInstruction);
            case OpCode::StoreOutput:
                return sizeof(StoreOutputInstruction);
            case OpCode::IsInf:
                return sizeof(IsInfInstruction);
            case OpCode::IsNaN:
                return sizeof(IsNaNInstruction);
            case OpCode::AtomicOr:
                return sizeof(AtomicOrInstruction);
            case OpCode::AtomicXOr:
                return sizeof(AtomicXOrInstruction);
            case OpCode::AtomicAnd:
                return sizeof(AtomicAndInstruction);
            case OpCode::AtomicAdd:
                return sizeof(AtomicAddInstruction);
            case OpCode::AtomicMin:
                return sizeof(AtomicMinInstruction);
            case OpCode::AtomicMax:
                return sizeof(AtomicMaxInstruction);
            case OpCode::AtomicExchange:
                return sizeof(AtomicExchangeInstruction);
            case OpCode::AtomicCompareExchange:
                return sizeof(AtomicCompareExchangeInstruction);
        }
    }
}
