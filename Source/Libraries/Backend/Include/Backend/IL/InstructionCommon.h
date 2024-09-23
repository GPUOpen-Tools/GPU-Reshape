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
#include "Instruction.h"

namespace IL {
    /// Get the control flow of an instruction
    /// \param instr given instruction
    /// \param out output control flow
    /// \return true if the instruction has control flow
    inline bool GetControlFlow(const Instruction* instr, BranchControlFlow& out) {
        switch (instr->opCode) {
            default: {
                return false;
            }
            case OpCode::Branch: {
                out = instr->As<::IL::BranchInstruction>()->controlFlow;
                return true;
            }
            case OpCode::BranchConditional: {
                out = instr->As<::IL::BranchConditionalInstruction>()->controlFlow;
                return true;
            }
            case OpCode::Switch: {
                out = instr->As<::IL::SwitchInstruction>()->controlFlow;
                return true;
            }
        }
    }

    /// Check if an instruction is a terminator
    inline bool IsTerminator(const Instruction* instr) {
        switch (instr->opCode) {
            default:
                return false;
            case OpCode::Return:
            case OpCode::Switch:
            case OpCode::Branch:
            case OpCode::BranchConditional:
                return true;
        }
    }

    /// Visit all operands of an instruction
    /// \param instr instruction to visit
    /// \param functor visitor
    template<typename F>
    inline void VisitOperands(const Instruction* instr, F&& functor) {
        switch (instr->opCode) {
            case OpCode::None: {
                break;
            }
            case OpCode::Unexposed: {
                auto typed = instr->As<::IL::UnexposedInstruction>();
                for (uint32_t i = 0; i < typed->operandCount; i++) {
                    functor(typed->operands[i]);
                }
                break;
            }
            case OpCode::Literal: {
                break;
            }
            case OpCode::Any: {
                auto typed = instr->As<::IL::AnyInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::All: {
                auto typed = instr->As<::IL::AllInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::Add: {
                auto typed = instr->As<::IL::AddInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Sub: {
                auto typed = instr->As<::IL::SubInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Div: {
                auto typed = instr->As<::IL::DivInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Mul: {
                auto typed = instr->As<::IL::MulInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Rem: {
                auto typed = instr->As<::IL::RemInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Trunc: {
                auto typed = instr->As<::IL::TruncInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::Or: {
                auto typed = instr->As<::IL::OrInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::And: {
                auto typed = instr->As<::IL::AndInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::Not: {
                auto typed = instr->As<::IL::NotInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::Equal: {
                auto typed = instr->As<::IL::EqualInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::NotEqual: {
                auto typed = instr->As<::IL::NotEqualInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::LessThan: {
                auto typed = instr->As<::IL::LessThanInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::LessThanEqual: {
                auto typed = instr->As<::IL::LessThanEqualInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::GreaterThan: {
                auto typed = instr->As<::IL::GreaterThanInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::GreaterThanEqual: {
                auto typed = instr->As<::IL::GreaterThanEqualInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::IsInf: {
                auto typed = instr->As<::IL::IsInfInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::IsNaN: {
                auto typed = instr->As<::IL::IsNaNInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::KernelValue: {
                break;
            }
            case OpCode::Extended: {
                auto typed = instr->As<::IL::ExtendedInstruction>();
                for (uint32_t i = 0; i < typed->operands.count; i++) {
                    functor(typed->operands[i]);
                }
                break;
            }
            case OpCode::Select: {
                auto typed = instr->As<::IL::SelectInstruction>();
                functor(typed->condition);
                functor(typed->pass);
                functor(typed->fail);
                break;
            }
            case OpCode::Branch: {
                break;
            }
            case OpCode::BranchConditional: {
                auto typed = instr->As<::IL::BranchConditionalInstruction>();
                functor(typed->cond);
                break;
            }
            case OpCode::Switch: {
                auto typed = instr->As<::IL::SwitchInstruction>();
                functor(typed->value);
                for (uint32_t i = 0; i < typed->cases.count; i++) {
                    functor(typed->cases[i].literal);
                }
                break;
            }
            case OpCode::Phi: {
                auto typed = instr->As<::IL::PhiInstruction>();
                for (uint32_t i = 0; i < typed->values.count; i++) {
                    functor(typed->values[i].value);
                }
                break;
            }
            case OpCode::Return: {
                auto typed = instr->As<::IL::ReturnInstruction>();
                if (typed->value != InvalidID) {
                    functor(typed->value);
                }
                break;
            }
            case OpCode::Call: {
                auto typed = instr->As<::IL::CallInstruction>();
                functor(typed->target);
                for (uint32_t i = 0; i < typed->arguments.count; i++) {
                    functor(typed->arguments[i]);
                }
                break;
            }
            case OpCode::AtomicOr: {
                auto typed = instr->As<::IL::AtomicOrInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicXOr: {
                auto typed = instr->As<::IL::AtomicXOrInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicAnd: {
                auto typed = instr->As<::IL::AtomicAndInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicAdd: {
                auto typed = instr->As<::IL::AtomicAddInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicMin: {
                auto typed = instr->As<::IL::AtomicMinInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicMax: {
                auto typed = instr->As<::IL::AtomicMaxInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::AtomicExchange: {
                auto typed = instr->As<::IL::AtomicExchangeInstruction>();
                functor(typed->value);
                functor(typed->address);
                break;
            }
            case OpCode::AtomicCompareExchange: {
                auto typed = instr->As<::IL::AtomicCompareExchangeInstruction>();
                functor(typed->value);
                functor(typed->address);
                functor(typed->comparator);
                break;
            }
            case OpCode::BitOr: {
                auto typed = instr->As<::IL::BitOrInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::BitXOr: {
                auto typed = instr->As<::IL::BitXOrInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::BitAnd: {
                auto typed = instr->As<::IL::BitAndInstruction>();
                functor(typed->lhs);
                functor(typed->rhs);
                break;
            }
            case OpCode::BitShiftLeft: {
                auto typed = instr->As<::IL::BitShiftLeftInstruction>();
                functor(typed->value);
                functor(typed->shift);
                break;
            }
            case OpCode::BitShiftRight: {
                auto typed = instr->As<::IL::BitShiftRightInstruction>();
                functor(typed->value);
                functor(typed->shift);
                break;
            }
            case OpCode::AddressChain: {
                auto typed = instr->As<::IL::AddressChainInstruction>();
                functor(typed->composite);
                for (uint32_t i = 0; i < typed->chains.count; i++) {
                    functor(typed->chains[i].index);
                }
                break;
            }
            case OpCode::Construct: {
                auto typed = instr->As<::IL::ConstructInstruction>();
                for (uint32_t i = 0; i < typed->values.count; i++) {
                    functor(typed->values[i]);
                }
                break;
            }
            case OpCode::Extract: {
                auto typed = instr->As<::IL::ExtractInstruction>();
                functor(typed->composite);
                for (uint32_t i = 0; i < typed->chains.count; i++) {
                    functor(typed->chains[i].index);
                }
                break;
            }
            case OpCode::Insert: {
                auto typed = instr->As<::IL::InsertInstruction>();
                functor(typed->composite);
                functor(typed->value);
                break;
            }
            case OpCode::FloatToInt: {
                auto typed = instr->As<::IL::FloatToIntInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::IntToFloat: {
                auto typed = instr->As<::IL::IntToFloatInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::BitCast: {
                auto typed = instr->As<::IL::BitCastInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::Export: {
                auto typed = instr->As<::IL::ExportInstruction>();
                for (uint32_t i = 0; i < typed->values.count; i++) {
                    functor(typed->values[i]);
                }
                break;
            }
            case OpCode::Alloca: {
                break;
            }
            case OpCode::Load: {
                auto typed = instr->As<::IL::LoadInstruction>();
                functor(typed->address);
                break;
            }
            case OpCode::Store: {
                auto typed = instr->As<::IL::StoreInstruction>();
                functor(typed->address);
                functor(typed->value);
                break;
            }
            case OpCode::StoreOutput: {
                auto typed = instr->As<::IL::StoreOutputInstruction>();
                functor(typed->value);
                functor(typed->row);
                functor(typed->column);
                functor(typed->index);
                break;
            }
            case OpCode::StoreVertexOutput: {
                auto typed = instr->As<::IL::StoreVertexOutputInstruction>();
                functor(typed->value);
                functor(typed->row);
                functor(typed->column);
                functor(typed->index);
                functor(typed->vertexIndex);
                break;
            }
            case OpCode::StorePrimitiveOutput: {
                auto typed = instr->As<::IL::StorePrimitiveOutputInstruction>();
                functor(typed->value);
                functor(typed->row);
                functor(typed->column);
                functor(typed->index);
                functor(typed->primitiveIndex);
                break;
            }
            case OpCode::SampleTexture: {
                auto typed = instr->As<::IL::SampleTextureInstruction>();
                functor(typed->texture);
                functor(typed->coordinate);

                if (typed->sampler != InvalidID) {
                    functor(typed->sampler);
                }

                if (typed->reference != InvalidID) {
                    functor(typed->reference);
                }

                if (typed->lod != InvalidID) {
                    functor(typed->lod);
                }

                if (typed->bias != InvalidID) {
                    functor(typed->bias);
                }

                if (typed->ddx != InvalidID) {
                    functor(typed->ddx);
                }

                if (typed->ddy != InvalidID) {
                    functor(typed->ddy);
                }
                break;
            }
            case OpCode::StoreTexture: {
                auto typed = instr->As<::IL::StoreTextureInstruction>();
                functor(typed->index);
                functor(typed->texel);
                functor(typed->texture);
                break;
            }
            case OpCode::LoadTexture: {
                auto typed = instr->As<::IL::LoadTextureInstruction>();
                functor(typed->texture);
                functor(typed->index);

                if (typed->offset != InvalidID) {
                    functor(typed->offset);
                }

                if (typed->mip != InvalidID) {
                    functor(typed->mip);
                }
                break;
            }
            case OpCode::StoreBuffer: {
                auto typed = instr->As<::IL::StoreBufferInstruction>();
                functor(typed->buffer);
                functor(typed->index);
                functor(typed->value);
                break;
            }
            case OpCode::StoreBufferRaw: {
                auto typed = instr->As<::IL::StoreBufferRawInstruction>();
                functor(typed->buffer);
                functor(typed->index);
                functor(typed->value);
                break;
            }
            case OpCode::LoadBuffer: {
                auto typed = instr->As<::IL::LoadBufferInstruction>();
                functor(typed->buffer);
                functor(typed->index);

                if (typed->offset != InvalidID) {
                    functor(typed->offset);
                }
                break;
            }
            case OpCode::LoadBufferRaw: {
                auto typed = instr->As<::IL::LoadBufferRawInstruction>();
                functor(typed->buffer);
                functor(typed->index);

                if (typed->offset != InvalidID) {
                    functor(typed->offset);
                }
                break;
            }
            case OpCode::ResourceToken: {
                auto typed = instr->As<::IL::ResourceTokenInstruction>();
                functor(typed->resource);
                break;
            }
            case OpCode::ResourceSize: {
                auto typed = instr->As<::IL::ResourceSizeInstruction>();
                functor(typed->resource);
                break;
            }
            case OpCode::WaveAnyTrue: {
                auto typed = instr->As<::IL::WaveAnyTrueInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveAllTrue: {
                auto typed = instr->As<::IL::WaveAllTrueInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveBallot: {
                auto typed = instr->As<::IL::WaveBallotInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveRead: {
                auto typed = instr->As<::IL::WaveReadInstruction>();
                functor(typed->value);
                functor(typed->lane);
                break;
            }
            case OpCode::WaveReadFirst: {
                auto typed = instr->As<::IL::WaveReadFirstInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveAllEqual: {
                auto typed = instr->As<::IL::WaveAllEqualInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveBitAnd: {
                auto typed = instr->As<::IL::WaveBitAndInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveBitOr: {
                auto typed = instr->As<::IL::WaveBitOrInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveBitXOr: {
                auto typed = instr->As<::IL::WaveBitXOrInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveCountBits: {
                auto typed = instr->As<::IL::WaveCountBitsInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveMax: {
                auto typed = instr->As<::IL::WaveMaxInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveMin: {
                auto typed = instr->As<::IL::WaveMinInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveProduct: {
                auto typed = instr->As<::IL::WaveProductInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WaveSum: {
                auto typed = instr->As<::IL::WaveSumInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WavePrefixCountBits: {
                auto typed = instr->As<::IL::WavePrefixCountBitsInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WavePrefixProduct: {
                auto typed = instr->As<::IL::WavePrefixProductInstruction>();
                functor(typed->value);
                break;
            }
            case OpCode::WavePrefixSum: {
                auto typed = instr->As<::IL::WavePrefixSumInstruction>();
                functor(typed->value);
                break;
            }
        }
    }
}
