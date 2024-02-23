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

#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/Program.h>
#include <Backend/IL/Function.h>
#include <Backend/IL/BasicBlock.h>

void IL::PrettyPrint(const Program &program, IL::PrettyPrintContext out) {
    out.Line() << "Program\n";
    out.Line() << "Bound     : " << program.GetIdentifierMap().GetMaxID() << "\n";
    out.Line() << "Functions : " << program.GetFunctionList().GetCount() << "\n";
    out.Line() << "\n";

    // Print all types
    PrettyPrint(program.GetTypeMap(), out);
    out.Line() << "\n";

    // Print all constants
    PrettyPrint(program.GetConstants(), out);
    out.Line() << "\n";

    // Print all variables
    PrettyPrint(&program, program.GetVariableList(), out);
    out.Line() << "\n";

    // Print all functions
    for (const IL::Function *fn: program.GetFunctionList()) {
        if (*program.GetFunctionList().begin() != fn) {
            out.Line() << "\n";
        }
        
        PrettyPrint(&program, *fn, out);
    }
}

void IL::PrettyPrint(const Program* program, const VariableList& variables, IL::PrettyPrintContext out) {
    out.stream << "Variables\n";
    out.TabInline();

    for (const Backend::IL::Variable* variable : variables) {
        out.Line() << "%" << variable->id << " = global ";
        PrettyPrint(variable->type, out);

        if (variable->initializer) {
            out.stream << " initializer:%" << variable->initializer->id;
        }
        
        out.stream << "\n";
    }
}

void IL::PrettyPrint(const Program *program, const Function &function, IL::PrettyPrintContext out) {
    out.Line() << "%" << function.GetID() << " = Function\n";

    // Print all basic blocks
    for (const IL::BasicBlock *bb: function.GetBasicBlocks()) {
        PrettyPrint(program, *bb, out.Tab());
    }
}

void IL::PrettyPrint(const Program *program, const BasicBlock &basicBlock, IL::PrettyPrintContext out) {
    out.Line() << "%" << basicBlock.GetID() << " = BasicBlock\n";

    // Print all instructions
    for (const IL::Instruction *instr: basicBlock) {
        PrettyPrint(program, instr, out.Tab());
    }
}


void IL::PrettyPrint(const Program *program, const Instruction *instr, IL::PrettyPrintContext out) {
    std::ostream &line = out.Line();

    if (instr->result != IL::InvalidID) {
        line << "%" << instr->result << " = ";
    }

    switch (instr->opCode) {
        default:
        ASSERT(false, "Unknown instruction");
            break;
        case OpCode::None: {
            line << "None";
            break;
        }
        case OpCode::Unexposed: {
            auto unexposed = instr->As<IL::UnexposedInstruction>();
            if (unexposed->symbol) {
                line << "Unexposed '" << unexposed->symbol << "'";
            } else {
                line << "Unexposed op:" << unexposed->backendOpCode;
            }

            for (uint32_t i = 0; i < unexposed->operandCount; i++) {
                line << " %" << unexposed->operands[i];
            }
            break;
        }
        case OpCode::Literal: {
            auto literal = instr->As<IL::LiteralInstruction>();
            line << "Literal ";

            switch (literal->type) {
                case LiteralType::None:
                    line << "None ";
                    break;
                case LiteralType::Int:
                    line << "Int";
                    break;
                case LiteralType::FP:
                    line << "FP";
                    break;
            }

            line << static_cast<uint32_t>(literal->bitWidth) << " ";

            if (literal->signedness) {
                line << "signed ";
            } else {
                line << "unsigned ";
            }

            switch (literal->type) {
                case LiteralType::None:
                    break;
                case LiteralType::Int:
                    line << "\"" << literal->value.integral << "\"";
                    break;
                case LiteralType::FP:
                    line << "\"" << literal->value.fp << "\"";
                    break;
            }
            break;
        }
        case OpCode::Add: {
            auto add = instr->As<IL::AddInstruction>();
            line << "Add %" << add->lhs << " %" << add->rhs;
            break;
        }
        case OpCode::Load: {
            auto load = instr->As<IL::LoadInstruction>();
            line << "Load %" << load->address;
            break;
        }
        case OpCode::Store: {
            auto store = instr->As<IL::StoreInstruction>();
            line << "Store address:%" << store->address << " value:%" << store->value;
            break;
        }
        case OpCode::SampleTexture: {
            auto load = instr->As<IL::SampleTextureInstruction>();
            line << "SampleTexture";

            switch (load->sampleMode) {
                default:
                    ASSERT(false, "Invalid sampler mode");
                    break;
                case Backend::IL::TextureSampleMode::Default:
                    break;
                case Backend::IL::TextureSampleMode::DepthComparison:
                    line << " DepthComparison";
                    break;
                case Backend::IL::TextureSampleMode::Projection:
                    line << " Projection";
                    break;
                case Backend::IL::TextureSampleMode::ProjectionDepthComparison:
                    line << " ProjectionDepthComparison";
                    break;
                case Backend::IL::TextureSampleMode::Unexposed:
                    line << " Unexposed";
                    break;
            }
            
            line << " texture:%" << load->texture;

            if (load->sampler != InvalidID) {
                line << " sampler:%" << load->sampler;
            }

            line << " coordinate:%" << load->coordinate;

            if (load->sampler != InvalidID) {
                line << " sampler:%" << load->sampler;
            }

            if (load->reference != InvalidID) {
                line << " sampler:%" << load->reference;
            }

            if (load->lod != InvalidID) {
                line << " sampler:%" << load->lod;
            }

            if (load->bias != InvalidID) {
                line << " sampler:%" << load->bias;
            }

            if (load->ddx != InvalidID) {
                line << " ddx:%" << load->ddx;
                line << " ddy:%" << load->ddy;
            }
            break;
        }
        case OpCode::LoadTexture: {
            auto load = instr->As<IL::LoadTextureInstruction>();
            line << "LoadTexture texture:%" << load->texture << " index:%" << load->index;
            break;
        }
        case OpCode::StoreBuffer: {
            auto store = instr->As<IL::StoreBufferInstruction>();
            line << "StoreBuffer buffer:%" << store->buffer << " index:%" << store->index << " value:%" << store->value;
            break;
        }
        case OpCode::StoreOutput: {
            auto store = instr->As<IL::StoreOutputInstruction>();
            line << "StoreOutput index:%" << store->index << " row:%" << store->row << " column:%" << store->column << " value:%" << store->value;
            break;
        }
        case OpCode::IsInf: {
            auto isInf = instr->As<IL::IsInfInstruction>();
            line << "IsInf %" << isInf->value;
            break;
        }
        case OpCode::IsNaN: {
            auto isNaN = instr->As<IL::IsNaNInstruction>();
            line << "IsNaN %" << isNaN->value;
            break;
        }
        case OpCode::Sub: {
            auto sub = instr->As<IL::SubInstruction>();
            line << "Sub %" << sub->lhs << " %" << sub->rhs;
            break;
        }
        case OpCode::Div: {
            auto sub = instr->As<IL::DivInstruction>();
            line << "Sub %" << sub->lhs << " %" << sub->rhs;
            break;
        }
        case OpCode::Mul: {
            auto mul = instr->As<IL::MulInstruction>();
            line << "Mul %" << mul->lhs << " %" << mul->rhs;
            break;
        }
        case OpCode::Or: {
            auto _or = instr->As<IL::OrInstruction>();
            line << "Or %" << _or->lhs << " %" << _or->rhs;
            break;
        }
        case OpCode::And: {
            auto _and = instr->As<IL::AndInstruction>();
            line << "And %" << _and->lhs << " %" << _and->rhs;
            break;
        }
        case OpCode::Equal: {
            auto equal = instr->As<IL::EqualInstruction>();
            line << "Equal %" << equal->lhs << " %" << equal->rhs;
            break;
        }
        case OpCode::NotEqual: {
            auto notEqual = instr->As<IL::NotEqualInstruction>();
            line << "NotEqual %" << notEqual->lhs << " %" << notEqual->rhs;
            break;
        }
        case OpCode::LessThan: {
            auto lessThan = instr->As<IL::LessThanInstruction>();
            line << "LessThan %" << lessThan->lhs << " %" << lessThan->rhs;
            break;
        }
        case OpCode::LessThanEqual: {
            auto lessThanEqual = instr->As<IL::LessThanEqualInstruction>();
            line << "LessThanEqual %" << lessThanEqual->lhs << " %" << lessThanEqual->rhs;
            break;
        }
        case OpCode::GreaterThan: {
            auto greaterThan = instr->As<IL::GreaterThanInstruction>();
            line << "GreaterThan %" << greaterThan->lhs << " %" << greaterThan->rhs;
            break;
        }
        case OpCode::GreaterThanEqual: {
            auto greaterThanEqual = instr->As<IL::GreaterThanEqualInstruction>();
            line << "GreaterThanEqual %" << greaterThanEqual->lhs << " %" << greaterThanEqual->rhs;
            break;
        }
        case OpCode::AddressChain: {
            auto addressChain = instr->As<IL::AddressChainInstruction>();
            line << "AddressChain composite:%" << addressChain->composite;

            for (uint32_t i = 0; i < addressChain->chains.count; i++) {
                line << "[ %" << addressChain->chains[i].index << " ]";
            }
            break;
        }
        case OpCode::Extract: {
            auto extract = instr->As<IL::ExtractInstruction>();
            line << "Extract composite:%" << extract->composite << " index:%" << extract->index;
            break;
        }
        case OpCode::Insert: {
            auto insert = instr->As<IL::InsertInstruction>();
            line << "Insert composite:%" << insert->composite << " value:%" << insert->value;
            break;
        }
        case OpCode::Select: {
            auto select = instr->As<IL::SelectInstruction>();
            line << "Select cond:%" << select->condition << " pass:%" << select->pass << " fail:%" << select->fail;
            break;
        }
        case OpCode::Branch: {
            auto branch = instr->As<IL::BranchInstruction>();
            line << "Branch %" << branch->branch;
            break;
        }
        case OpCode::BranchConditional: {
            auto branch = instr->As<IL::BranchConditionalInstruction>();
            line << "BranchConditional cond:%" << branch->cond << " pass:%" << branch->pass << " fail:%" << branch->fail;

            // Optional control flow
            if (branch->controlFlow.merge != InvalidID) {
                line << " control:{merge:%" << branch->controlFlow.merge;

                if (branch->controlFlow._continue != InvalidID) {
                    line << ", continue:%" << branch->controlFlow._continue;
                }

                line << "}";
            }

            break;
        }
        case OpCode::Switch: {
            auto _switch = instr->As<IL::SwitchInstruction>();
            line << "Switch value:%" << _switch->value << " default:%" << _switch->_default;

            // Optional control flow
            if (_switch->controlFlow.merge != InvalidID) {
                line << " control:{merge:%" << _switch->controlFlow.merge;

                if (_switch->controlFlow._continue != InvalidID) {
                    line << ", continue:%" << _switch->controlFlow._continue;
                }

                line << "}";
            }

            for (uint32_t i = 0; i < _switch->cases.count; i++) {
                const IL::SwitchCase &_case = _switch->cases[i];
                line << "\n";
                out.Line() << "       literal:" << _case.literal << " branch:%" << _case.branch;
            }
            break;
        }
        case OpCode::Phi: {
            auto phi = instr->As<IL::PhiInstruction>();
            line << "Phi ";

            for (uint32_t i = 0; i < phi->values.count; i++) {
                const IL::PhiValue &_case = phi->values[i];
                line << "\n";
                out.Line() << "       value:%" << _case.value << " branch:%" << _case.branch;
            }
            break;
        }
        case OpCode::BitOr: {
            auto bitOr = instr->As<IL::BitOrInstruction>();
            line << "BitOr %" << bitOr->lhs << " %" << bitOr->rhs;
            break;
        }
        case OpCode::BitAnd: {
            auto bitAnd = instr->As<IL::BitAndInstruction>();
            line << "BitAnd %" << bitAnd->lhs << " %" << bitAnd->rhs;
            break;
        }
        case OpCode::BitShiftLeft: {
            auto bitShiftLeft = instr->As<IL::BitShiftLeftInstruction>();
            line << "BitShiftLeft %" << bitShiftLeft->value << " shift:%" << bitShiftLeft->shift;
            break;
        }
        case OpCode::BitShiftRight: {
            auto bitShiftRight = instr->As<IL::BitShiftRightInstruction>();
            line << "BitShiftRight %" << bitShiftRight->value << " shift:%" << bitShiftRight->shift;
            break;
        }
        case OpCode::Export: {
            auto _export = instr->As<IL::ExportInstruction>();
            line << "Export values:[";

            for (uint32_t i = 0; i < _export->values.count; i++) {
                if (i != 0) {
                    line << ", ";
                }

                line << "%" << _export->values[i];
            }

            line << "] exportID:" << _export->exportID;
            break;
        }
        case OpCode::Alloca: {
            auto _alloca = instr->As<IL::AllocaInstruction>();
            line << "Alloca ";

            if (program) {
                PrettyPrint(program->GetTypeMap().GetType(_alloca->result)->As<Backend::IL::PointerType>()->pointee, out);
            }
            break;
        }
        case OpCode::StoreTexture: {
            auto store = instr->As<IL::StoreTextureInstruction>();
            line << "StoreTexture texture:%" << store->texture << " index:%" << store->index << " texel:%" << store->texel;
            break;
        }
        case OpCode::Any: {
            auto _or = instr->As<IL::AnyInstruction>();
            line << "Any %" << _or->value;
            break;
        }
        case OpCode::All: {
            auto load = instr->As<IL::AllInstruction>();
            line << "All %" << load->value;
            break;
        }
        case OpCode::LoadBuffer: {
            auto load = instr->As<IL::LoadBufferInstruction>();
            line << "LoadBuffer buffer:%" << load->buffer << " index:%" << load->index;

            if (load->offset != InvalidID) {
                line << " offset:%" << load->offset;
            }
            break;
        }
        case OpCode::ResourceSize: {
            auto load = instr->As<IL::ResourceSizeInstruction>();
            line << "ResourceSize %" << load->resource;
            break;
        }
        case OpCode::ResourceToken: {
            auto load = instr->As<IL::ResourceTokenInstruction>();
            line << "ResourceToken %" << load->resource;
            break;
        }
        case OpCode::Rem: {
            auto rem = instr->As<IL::RemInstruction>();
            line << "Rem %" << rem->lhs << " %" << rem->rhs;
            break;
        }
        case OpCode::Trunc: {
            auto trunc = instr->As<IL::TruncInstruction>();
            line << "Trunc %" << trunc->value;
            break;
        }
        case OpCode::Return: {
            auto ret = instr->As<IL::ReturnInstruction>();

            if (ret->value == IL::InvalidID) {
                line << "Return";
            } else {
                line << "Return %" << ret->value;
            }
            break;
        }
        case OpCode::BitXOr: {
            auto _xor = instr->As<IL::BitXOrInstruction>();
            line << "XOr %" << _xor->lhs << " %" << _xor->rhs;
            break;
        }
        case OpCode::FloatToInt: {
            auto ftoi = instr->As<IL::FloatToIntInstruction>();
            line << "FloatToInt %" << ftoi->value;
            break;
        }
        case OpCode::IntToFloat: {
            auto itof = instr->As<IL::IntToFloatInstruction>();
            line << "IntToFloat %" << itof->value;
            break;
        }
        case OpCode::BitCast: {
            auto cast = instr->As<IL::BitCastInstruction>();
            line << "BitCast %" << cast->value;
            break;
        }
        case OpCode::AtomicOr: {
            auto atomic = instr->As<IL::AtomicOrInstruction>();
            line << "AtomicOr address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicXOr: {
            auto atomic = instr->As<IL::AtomicXOrInstruction>();
            line << "AtomicXOr address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicAnd: {
            auto atomic = instr->As<IL::AtomicAndInstruction>();
            line << "AtomicAnd address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicAdd: {
            auto atomic = instr->As<IL::AtomicAddInstruction>();
            line << "AtomicAdd address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicMin: {
            auto atomic = instr->As<IL::AtomicMinInstruction>();
            line << "AtomicMin address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicMax: {
            auto atomic = instr->As<IL::AtomicMinInstruction>();
            line << "AtomicMax address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicExchange: {
            auto atomic = instr->As<IL::AtomicExchangeInstruction>();
            line << "AtomicExchange address:%" << atomic->address << " value:%" << atomic->value;
            break;
        }
        case OpCode::AtomicCompareExchange: {
            auto atomic = instr->As<IL::AtomicCompareExchangeInstruction>();
            line << "AtomicCompareExchange address:%" << atomic->address << " value:%" << atomic->value << " comparator:%" << atomic->comparator;
            break;
        }
        case OpCode::WaveAnyTrue: {
            auto atomic = instr->As<IL::WaveAnyTrueInstruction>();
            line << "WaveAnyTrue value:%" << atomic->value;
            break;
        }
        case OpCode::WaveAllTrue: {
            auto atomic = instr->As<IL::WaveAllTrueInstruction>();
            line << "WaveAllTrue value:%" << atomic->value;
            break;
        }
        case OpCode::WaveBallot: {
            auto atomic = instr->As<IL::WaveBallotInstruction>();
            line << "WaveBallot value:%" << atomic->value;
            break;
        }
        case OpCode::WaveRead: {
            auto atomic = instr->As<IL::WaveReadInstruction>();
            line << "WaveRead value:%" << atomic->value << " lane:%" << atomic->lane;
            break;
        }
        case OpCode::WaveReadFirst: {
            auto atomic = instr->As<IL::WaveReadFirstInstruction>();
            line << "WaveReadFirst value:%" << atomic->value;
            break;
        }
        case OpCode::WaveAllEqual: {
            auto atomic = instr->As<IL::WaveAllEqualInstruction>();
            line << "WaveAllEqual value:%" << atomic->value;
            break;
        }
        case OpCode::WaveBitAnd: {
            auto atomic = instr->As<IL::WaveBitAndInstruction>();
            line << "WaveBitAnd value:%" << atomic->value;
            break;
        }
        case OpCode::WaveBitOr: {
            auto atomic = instr->As<IL::WaveBitOrInstruction>();
            line << "WaveBitOr value:%" << atomic->value;
            break;
        }
        case OpCode::WaveBitXOr: {
            auto atomic = instr->As<IL::WaveBitXOrInstruction>();
            line << "WaveBitXOr value:%" << atomic->value;
            break;
        }
        case OpCode::WaveCountBits: {
            auto atomic = instr->As<IL::WaveCountBitsInstruction>();
            line << "WaveCountBits value:%" << atomic->value;
            break;
        }
        case OpCode::WaveMax: {
            auto atomic = instr->As<IL::WaveMaxInstruction>();
            line << "WaveMax value:%" << atomic->value;
            break;
        }
        case OpCode::WaveMin: {
            auto atomic = instr->As<IL::WaveMinInstruction>();
            line << "WaveMin value:%" << atomic->value;
            break;
        }
        case OpCode::WaveProduct: {
            auto atomic = instr->As<IL::WaveProductInstruction>();
            line << "WaveProduct value:%" << atomic->value;
            break;
        }
        case OpCode::WaveSum: {
            auto atomic = instr->As<IL::WaveSumInstruction>();
            line << "WaveSum value:%" << atomic->value;
            break;
        }
        case OpCode::WavePrefixCountBits: {
            auto atomic = instr->As<IL::WavePrefixCountBitsInstruction>();
            line << "WavePrefixCountBits value:%" << atomic->value;
            break;
        }
        case OpCode::WavePrefixProduct: {
            auto atomic = instr->As<IL::WavePrefixProductInstruction>();
            line << "WavePrefixProduct value:%" << atomic->value;
            break;
        }
        case OpCode::WavePrefixSum: {
            auto atomic = instr->As<IL::WavePrefixSumInstruction>();
            line << "WavePrefixSum value:%" << atomic->value;
            break;
        }
    }

    if (instr->source.IsValid()) {
        line << " [" << instr->source.codeOffset << "]";
    }

    line << "\n";
}

void IL::PrettyPrint(Backend::IL::Format format, PrettyPrintContext out) {
    std::ostream &line = out.stream;

    switch (format) {
        default:
        ASSERT(false, "Unexpected format");
            break;
        case Backend::IL::Format::None:
            line << "None";
            break;
        case Backend::IL::Format::RGBA32Float:
            line << "RGBA32Float";
            break;
        case Backend::IL::Format::RGBA16Float:
            line << "RGBA16Float";
            break;
        case Backend::IL::Format::R32Float:
            line << "R32Float";
            break;
        case Backend::IL::Format::R32Unorm:
            line << "R32UNorm";
            break;
        case Backend::IL::Format::RGBA8:
            line << "RGBA8";
            break;
        case Backend::IL::Format::RGBA8Snorm:
            line << "RGBA8Snorm";
            break;
        case Backend::IL::Format::RG32Float:
            line << "RG32Float";
            break;
        case Backend::IL::Format::RG16Float:
            line << "RG16Float";
            break;
        case Backend::IL::Format::R11G11B10Float:
            line << "R11G11B10Float";
            break;
        case Backend::IL::Format::R16Unorm:
            line << "R16Unorm";
            break;
        case Backend::IL::Format::R16Float:
            line << "R16Float";
            break;
        case Backend::IL::Format::RGBA16:
            line << "RGBA16";
            break;
        case Backend::IL::Format::RGB10A2:
            line << "RGB10A2";
            break;
        case Backend::IL::Format::RG16:
            line << "RG16";
            break;
        case Backend::IL::Format::RG8:
            line << "RG8";
            break;
        case Backend::IL::Format::R16:
            line << "R16";
            break;
        case Backend::IL::Format::R8:
            line << "R8";
            break;
        case Backend::IL::Format::RGBA16Snorm:
            line << "RGBA16Snorm";
            break;
        case Backend::IL::Format::RG16Snorm:
            line << "RG16Snorm";
            break;
        case Backend::IL::Format::RG8Snorm:
            line << "RG8Snorm";
            break;
        case Backend::IL::Format::R16Snorm:
            line << "R16Snorm";
            break;
        case Backend::IL::Format::R8Snorm:
            line << "R8Snorm";
            break;
        case Backend::IL::Format::RGBA32Int:
            line << "RGBA32Int";
            break;
        case Backend::IL::Format::RGBA16Int:
            line << "RGBA16Int";
            break;
        case Backend::IL::Format::RGBA8Int:
            line << "RGBA8Int";
            break;
        case Backend::IL::Format::R32Int:
            line << "R32Int";
            break;
        case Backend::IL::Format::RG32Int:
            line << "RG32Int";
            break;
        case Backend::IL::Format::RG16Int:
            line << "RG16Int";
            break;
        case Backend::IL::Format::RG8Int:
            line << "RG8Int";
            break;
        case Backend::IL::Format::R16Int:
            line << "R16Int";
            break;
        case Backend::IL::Format::R8Int:
            line << "R8Int";
            break;
        case Backend::IL::Format::RGBA32UInt:
            line << "RGBA32UInt";
            break;
        case Backend::IL::Format::RGBA16UInt:
            line << "RGBA16UInt";
            break;
        case Backend::IL::Format::RGBA8UInt:
            line << "RGBA8UInt";
            break;
        case Backend::IL::Format::R32UInt:
            line << "R32UInt";
            break;
        case Backend::IL::Format::RGB10a2UInt:
            line << "RGB10a2UInt";
            break;
        case Backend::IL::Format::RG32UInt:
            line << "RG32UInt";
            break;
        case Backend::IL::Format::RG16UInt:
            line << "RG16UInt";
            break;
        case Backend::IL::Format::RG8UInt:
            line << "RG8UInt";
            break;
        case Backend::IL::Format::R16UInt:
            line << "R16UInt";
            break;
        case Backend::IL::Format::R8UInt:
            line << "R8UInt";
            break;
        case Backend::IL::Format::Unexposed:
            line << "Unexposed";
            break;
    }
}

void IL::PrettyPrint(Backend::IL::ResourceSamplerMode mode, PrettyPrintContext out) {
    std::ostream &line = out.stream;

    switch (mode) {
        case Backend::IL::ResourceSamplerMode::RuntimeOnly:
            line << "RuntimeOnly";
            break;
        case Backend::IL::ResourceSamplerMode::Compatible:
            line << "Compatible";
            break;
        case Backend::IL::ResourceSamplerMode::Writable:
            line << "Writable";
            break;
    }
}

void IL::PrettyPrint(const Backend::IL::Type *type, PrettyPrintContext out) {
    std::ostream &line = out.stream;
    switch (type->kind) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::TypeKind::Bool: {
            line << "Bool";
            break;
        }
        case Backend::IL::TypeKind::Void: {
            line << "Void";
            break;
        }
        case Backend::IL::TypeKind::Int: {
            auto _int = type->As<Backend::IL::IntType>();

            if (_int->signedness) {
                line << "Int";
            } else {
                line << "UInt";
            }

            line << " width:" << static_cast<uint32_t>(_int->bitWidth);
            break;
        }
        case Backend::IL::TypeKind::FP: {
            auto fp = type->As<Backend::IL::FPType>();
            line << "FP width:" << static_cast<uint32_t>(fp->bitWidth);
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto vec = type->As<Backend::IL::VectorType>();
            line << "Vector" << " contained:%" << vec->containedType->id << " dim:" << static_cast<uint32_t>(vec->dimension);
            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto mat = type->As<Backend::IL::MatrixType>();
            line << "Matrix" << " contained:%" << mat->containedType->id << " rows:" << static_cast<uint32_t>(mat->rows) << " columns:" << static_cast<uint32_t>(mat->columns);
            break;
        }
        case Backend::IL::TypeKind::Pointer: {
            auto ptr = type->As<Backend::IL::PointerType>();
            line << "Pointer pointee:"; 
            
            if (ptr->pointee) {
                line << "%" << ptr->pointee->id;
            } else {
                line << "forward";
            }

            line << " space:";
            
            switch (ptr->addressSpace) {
                case Backend::IL::AddressSpace::Constant:
                    line << "Constant";
                    break;
                case Backend::IL::AddressSpace::Texture:
                    line << "Texture";
                    break;
                case Backend::IL::AddressSpace::Buffer:
                    line << "Buffer";
                    break;
                case Backend::IL::AddressSpace::Function:
                    line << "Function";
                    break;
                case Backend::IL::AddressSpace::Resource:
                    line << "Resource";
                    break;
                case Backend::IL::AddressSpace::GroupShared:
                    line << "GroupShared";
                    break;
                case Backend::IL::AddressSpace::Unexposed:
                    line << "Unexposed";
                    break;
                case Backend::IL::AddressSpace::RootConstant:
                    line << "RootConstant";
                    break;
                case Backend::IL::AddressSpace::Output:
                    line << "Output";
                    break;
            }
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto arr = type->As<Backend::IL::ArrayType>();
            line << "Array" << " element:%" << arr->elementType->id << " count:" << arr->count;
            break;
        }
        case Backend::IL::TypeKind::Texture: {
            auto tex = type->As<Backend::IL::TextureType>();
            line << "Texture dim:";

            switch (tex->dimension) {
                case Backend::IL::TextureDimension::Texture1D:
                    line << "Texture1D";
                    break;
                case Backend::IL::TextureDimension::Texture2D:
                    line << "Texture2D";
                    break;
                case Backend::IL::TextureDimension::Texture3D:
                    line << "Texture3D";
                    break;
                case Backend::IL::TextureDimension::Texture1DArray:
                    line << "Texture1DArray";
                    break;
                case Backend::IL::TextureDimension::Texture2DArray:
                    line << "Texture2DArray";
                    break;
                case Backend::IL::TextureDimension::Texture2DCube:
                    line << "Texture2DCube";
                    break;
                case Backend::IL::TextureDimension::Texture2DCubeArray:
                    line << "Texture2DCubeArray";
                    break;
                case Backend::IL::TextureDimension::Unexposed:
                    line << "Unexposed";
                    break;
                case Backend::IL::TextureDimension::SubPass:
                    line << "SubPass";
                    break;
            }

            if (tex->sampledType) {
                line << " sampledType:%" << tex->sampledType->id;
            }

            line << " format:";
            PrettyPrint(tex->format, out);

            line << " samplerMode:";
            PrettyPrint(tex->samplerMode, out);

            if (tex->multisampled) {
                line << " multisampled";
            }
            break;
        }
        case Backend::IL::TypeKind::Buffer: {
            auto buf = type->As<Backend::IL::BufferType>();
            line << "Buffer format:";

            if (buf->elementType) {
                line << " elementType:%" << buf->elementType->id;
            }

            if (buf->texelType != Backend::IL::Format::None) {
                line << " texelType:";
                PrettyPrint(buf->texelType, out);
            }

            line << " samplerMode:";
            PrettyPrint(buf->samplerMode, out);
            break;
        }
        case Backend::IL::TypeKind::Sampler: {
            line << "Sampler";
            break;
        }
        case Backend::IL::TypeKind::CBuffer: {
            line << "CBuffer";
            break;
        }
        case Backend::IL::TypeKind::Function: {
            auto fn = type->As<Backend::IL::FunctionType>();
            line << "Function return:%" << fn->returnType->id << " parameters:[";

            for (size_t i = 0; i < fn->parameterTypes.size(); i++) {
                if (i != 0) {
                    line << ", ";
                }

                line << "%" << fn->parameterTypes[i]->id;
            }

            line << "]";
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto str = type->As<Backend::IL::StructType>();
            line << "Struct members:[";

            for (size_t i = 0; i < str->memberTypes.size(); i++) {
                if (i != 0) {
                    line << ", ";
                }

                line << "%" << str->memberTypes[i]->id;
            }

            line << "]";
            break;
        }
        case Backend::IL::TypeKind::Unexposed: {
            line << "Unexposed";
            break;
        }
    }
}

void IL::PrettyPrint(const Backend::IL::TypeMap &map, PrettyPrintContext out) {
    out.stream << "Type\n";
    out.TabInline();

    for (Backend::IL::Type *type: map) {
        std::ostream &line = out.Line();
        line << "%" << type->id << " = ";

        PrettyPrint(type, out);
        line << "\n";
    }
}

void IL::PrettyPrint(const Backend::IL::ConstantMap &map, PrettyPrintContext out) {
    out.stream << "Constant\n";
    out.TabInline();

    for (Backend::IL::Constant *constant: map) {
        std::ostream &line = out.Line();

        if (!constant) {
            line << "null\n";
            continue;
        }

        if (constant->IsSymbolic()) {
            continue;
        }

        line << "%" << constant->id << " = type:%" << constant->type->id << " ";
        PrettyPrint(constant, out);
        line << "\n";
    }
}

void IL::PrettyPrint(const IL::Constant* constant, PrettyPrintContext out) {
    switch (constant->kind) {
        default: {
            out.stream << "Unexposed";
            break;
        }
        case Backend::IL::ConstantKind::Bool: {
            auto _bool = constant->As<Backend::IL::BoolConstant>();
            out.stream << "Bool ";
            if (_bool->value) {
                out.stream << "true";
            } else {
                out.stream << "false";
            }
            break;
        }
        case Backend::IL::ConstantKind::Int: {
            auto _int = constant->As<Backend::IL::IntConstant>();

            if (_int->type->As<Backend::IL::IntType>()->signedness) {
                out.stream << "Int";
            } else {
                out.stream << "UInt";
            }

            out.stream << " " << _int->value;
            break;
        }
        case Backend::IL::ConstantKind::FP: {
            auto fp = constant->As<Backend::IL::FPConstant>();
            out.stream << "FP";
            out.stream << " " << fp->value;
            break;
        }
        case Backend::IL::ConstantKind::Array: {
            auto array = constant->As<Backend::IL::ArrayConstant>();
            out.stream << "Array [";

            for (size_t i = 0; i < array->elements.size(); i++) {
                if (i != 0) {
                    out.stream << ", ";
                }

                out.stream << "%" << array->elements[i]->id;
            }
            
            out.stream << "]";
            break;
        }
        case Backend::IL::ConstantKind::Struct: {
            auto _struct = constant->As<Backend::IL::StructConstant>();
            out.stream << "Struct {";

            for (size_t i = 0; i < _struct->members.size(); i++) {
                if (i != 0) {
                    out.stream << ", ";
                }

                out.stream << "%" << _struct->members[i]->id;
            }
            
            out.stream << "}";
            break;
        }
        case Backend::IL::ConstantKind::Undef: {
            PrettyPrint(constant->type, out);
            out.stream << " Undef";
            break;
        }
    }
}


static void PrettyPrintBlockDotGraphSuccessor(const IL::BasicBlockList &basicBlocks, const IL::BasicBlock *block, IL::ID successor, IL::PrettyPrintContext &out) {
    IL::BasicBlock *successorBlock = basicBlocks.GetBlock(successor);
    ASSERT(successorBlock, "Successor block invalid");

    // Must have terminator
    auto terminator = successorBlock->GetTerminator();
    ASSERT(terminator, "Must have terminator");

    // Get control flow, if present
    IL::BranchControlFlow controlFlow;
    switch (terminator.GetOpCode()) {
        default:
            break;
        case IL::OpCode::Branch:
            controlFlow = terminator.As<IL::BranchInstruction>()->controlFlow;
            break;
        case IL::OpCode::BranchConditional:
            controlFlow = terminator.As<IL::BranchConditionalInstruction>()->controlFlow;
            break;
    }

    // Skip loop back continue block for order resolving
    if (controlFlow._continue == block->GetID()) {
        out.stream << " n" << block->GetID() << " -> n" << successor << " [color=\"0 0 0.75\"];\n";
    } else {
        out.stream << " n" << block->GetID() << " -> n" << successor << ";\n";
    }
}

void IL::PrettyPrintBlockDotGraph(const Function &function, PrettyPrintContext out) {
    out.stream << "digraph regexp { \n"
                  " fontname=\"Helvetica,Arial,sans-serif\"\n";

    const BasicBlockList &blocks = function.GetBasicBlocks();

    for (auto it = blocks.begin(); it != blocks.end(); it++) {
        out.stream << " n" << (*it)->GetID();

        if (it == blocks.begin())
            out.stream << " [label=\"Entry\"";
        else
            out.stream << " [label=\"Block " << (*it)->GetID() << "\"";

        if ((*it)->GetFlags() & BasicBlockFlag::NoInstrumentation) {
            out.stream << ", style=filled, fillcolor=red";
        }

        out.stream << "];\n";
    }

    for (const IL::BasicBlock *bb: function.GetBasicBlocks()) {
        // Must have terminator
        auto terminator = bb->GetTerminator();
        ASSERT(terminator, "Must have terminator");

        // Get control flow, if present
        IL::BranchControlFlow controlFlow;
        switch (terminator.GetOpCode()) {
            default:
                break;
            case IL::OpCode::Branch:
                controlFlow = terminator.As<IL::BranchInstruction>()->controlFlow;
                break;
            case IL::OpCode::BranchConditional:
                controlFlow = terminator.As<IL::BranchConditionalInstruction>()->controlFlow;
                break;
        }

        for (auto &&instr: *bb) {
            switch (instr->opCode) {
                default:
                    break;
                case IL::OpCode::Branch: {
                    PrettyPrintBlockDotGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchInstruction>()->branch, out);
                    break;
                }
                case IL::OpCode::BranchConditional: {
                    PrettyPrintBlockDotGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchConditionalInstruction>()->pass, out);
                    PrettyPrintBlockDotGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchConditionalInstruction>()->fail, out);
                    break;
                }
                case IL::OpCode::Switch: {
                    auto *_switch = instr->As<IL::SwitchInstruction>();
                    out.stream << " n" << bb->GetID() << " -> n" << _switch->_default << " [color=\"0.85 0.5 0.25\"];\n";

                    // Add cases
                    for (uint32_t i = 0; i < _switch->cases.count; i++) {
                        out.stream << " n" << bb->GetID() << " -> n" << _switch->cases[i].branch << " [color=\"0.85 0.5 0.25\"];\n";
                    }
                    break;
                }
                case IL::OpCode::Phi: {
                    auto *phi = instr->As<IL::PhiInstruction>();

                    // Add producers for values
                    for (uint32_t i = 0; i < phi->values.count; i++) {
                        if (controlFlow._continue == phi->values[i].branch) {
                            continue;
                        }

                        out.stream << " n" << phi->values[i].branch << " -> n" << bb->GetID() << " [color=\"0.8 0.25 0.9\"];\n";
                    }
                    break;
                }
            }
        }
    }

    out.stream << "}\n";
}

struct BlockJsonEdge {
    IL::ID from;
    IL::ID to;
    std::string_view color = "";
};

static void PrettyPrintBlockJsonGraphSuccessor(const IL::BasicBlockList &basicBlocks, const IL::BasicBlock *block, IL::ID successor, std::vector<BlockJsonEdge> &edges) {
    IL::BasicBlock *successorBlock = basicBlocks.GetBlock(successor);
    ASSERT(successorBlock, "Successor block invalid");

    // Must have terminator
    auto terminator = successorBlock->GetTerminator();
    ASSERT(terminator, "Must have terminator");

    // Get control flow, if present
    IL::BranchControlFlow controlFlow;
    switch (terminator.GetOpCode()) {
        default:
            break;
        case IL::OpCode::Branch:
            controlFlow = terminator.As<IL::BranchInstruction>()->controlFlow;
        break;
        case IL::OpCode::BranchConditional:
            controlFlow = terminator.As<IL::BranchConditionalInstruction>()->controlFlow;
        break;
    }

    // Skip loop back continue block for order resolving
    if (controlFlow._continue == block->GetID()) {
        edges.push_back(BlockJsonEdge {
            .from = block->GetID(),
            .to = successor,
            .color = "gray"
        });
    } else {
        edges.push_back(BlockJsonEdge {
            .from = block->GetID(),
            .to = successor,
            .color = ""
        });
    }
}

void IL::PrettyPrintBlockJsonGraph(const Function &function, PrettyPrintContext out) {
    const BasicBlockList &blocks = function.GetBasicBlocks();

    // Begin document
    out.stream << "{\n";

    // Open blocks
    out.stream << "\t\"blocks\":\n";
    out.stream << "\t{\n";

    // Print all blocks
    for (auto it = blocks.begin(); it != blocks.end(); it++) {
        // Open
        out.stream << "\t\t\"" << (*it)->GetID() << "\" : {\n";

        // Special names
        if (it == blocks.begin()) {
            out.stream << "\t\t\t\"name\": \"EntryPoint\",\n";
        } else {
            out.stream << "\t\t\t\"name\": \"Block " << (*it)->GetID() << "\",\n";
        }

        // Special colors
        if ((*it)->GetFlags() & BasicBlockFlag::NoInstrumentation) {
            out.stream << "\t\t\t\"color\": \"red\"\n";
        } else {
            out.stream << "\t\t\t\"color\": \"white\"\n";
        }

        // Close
        out.stream << "\t\t}";

        if (it != --blocks.end()) {
            out.stream << ",";
        }
        
        out.stream << "\n";
    }

    // Close blocks
    out.stream << "\t},\n";

    // All edges
    std::vector<BlockJsonEdge> edges;

    // Populate edges
    for (const IL::BasicBlock *bb: function.GetBasicBlocks()) {
        // Must have terminator
        auto terminator = bb->GetTerminator();
        ASSERT(terminator, "Must have terminator");

        // Get control flow, if present
        IL::BranchControlFlow controlFlow;
        switch (terminator.GetOpCode()) {
            default:
                break;
            case IL::OpCode::Branch:
                controlFlow = terminator.As<IL::BranchInstruction>()->controlFlow;
                break;
            case IL::OpCode::BranchConditional:
                controlFlow = terminator.As<IL::BranchConditionalInstruction>()->controlFlow;
                break;
        }

        for (auto &&instr: *bb) {
            switch (instr->opCode) {
                default:
                    break;
                case IL::OpCode::Branch: {
                    PrettyPrintBlockJsonGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchInstruction>()->branch, edges);
                    break;
                }
                case IL::OpCode::BranchConditional: {
                    PrettyPrintBlockJsonGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchConditionalInstruction>()->pass, edges);
                    PrettyPrintBlockJsonGraphSuccessor(function.GetBasicBlocks(), bb, instr->As<IL::BranchConditionalInstruction>()->fail, edges);
                    break;
                }
                case IL::OpCode::Switch: {
                    auto *_switch = instr->As<IL::SwitchInstruction>();
                    edges.push_back(BlockJsonEdge {
                        .from = bb->GetID(),
                        .to = _switch->_default,
                        .color = "orange"
                    });

                    // Add cases
                    for (uint32_t i = 0; i < _switch->cases.count; i++) {
                        edges.push_back(BlockJsonEdge {
                            .from = bb->GetID(),
                            .to = _switch->cases[i].branch,
                            .color = "orange"
                        });
                    }
                    break;
                }
                case IL::OpCode::Phi: {
                    auto *phi = instr->As<IL::PhiInstruction>();

                    // Add producers for values
                    for (uint32_t i = 0; i < phi->values.count; i++) {
                        if (controlFlow._continue == phi->values[i].branch) {
                            continue;
                        }

                        edges.push_back(BlockJsonEdge {
                            .from = phi->values[i].branch,
                            .to = bb->GetID(),
                            .color = "violet"
                        });
                    }
                    break;
                }
            }
        }
    }

    // Open edge list
    out.stream << "\t\"edges\":\n";
    out.stream << "\t[\n";

    // Print edges
    for (auto it = edges.begin(); it != edges.end(); it++) {
        out.stream << "\t\t{\n";
        out.stream << "\t\t\t\"from\": " << it->from << ",\n";
        out.stream << "\t\t\t\"to\": " << it->to << ",\n";
        out.stream << "\t\t\t\"color\": \"" << it->color << "\"\n";
        out.stream << "\t\t}";

        if (it != --edges.end()) {
            out.stream << ",";
        }

        out.stream << "\n";
    }

    // Close edges
    out.stream << "\t]\n";

    // Close document
    out.stream << "}\n";
}

void PrettyPrintJson(const Backend::IL::Type *type, IL::PrettyPrintContext out) {
    switch (type->kind) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::TypeKind::Bool: {
            break;
        }
        case Backend::IL::TypeKind::Void: {
            break;
        }
        case Backend::IL::TypeKind::Int: {
            auto _int = type->As<Backend::IL::IntType>();

            out.Line() << "\"Signed\": " << _int->signedness << ",";
            out.Line() << "\"Width\": " << static_cast<uint32_t>(_int->bitWidth) << ",";
            break;
        }
        case Backend::IL::TypeKind::FP: {
            auto fp = type->As<Backend::IL::FPType>();
            out.Line() << "\"Width\": " << static_cast<uint32_t>(fp->bitWidth) << ",";
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto vec = type->As<Backend::IL::VectorType>();
            out.Line() << "\"Contained\": " << vec->containedType->id << ",";
            out.Line() << "\"Dim\": " << static_cast<uint32_t>(vec->dimension) << ",";
            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto mat = type->As<Backend::IL::MatrixType>();
            out.Line() << "\"Contained\": " << mat->containedType->id << ",";
            out.Line() << "\"Rows\": " << static_cast<uint32_t>(mat->rows) << ",";
            out.Line() << "\"Columns\": " << static_cast<uint32_t>(mat->columns) << ",";
            break;
        }
        case Backend::IL::TypeKind::Pointer: {
            auto ptr = type->As<Backend::IL::PointerType>();
            out.Line() << "\"AddressSpace\": " << static_cast<uint32_t>(ptr->addressSpace) << ",";
            out.Line() << "\"Pointee\": ";

            if (ptr->pointee) {
                out.stream << ptr->pointee->id;
            } else {
                out.stream << IL::InvalidID;
            }
            
            out.stream << ",";
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto arr = type->As<Backend::IL::ArrayType>();
            out.Line() << "\"Contained\": " << arr->elementType->id << ",";
            out.Line() << "\"Count\": " << arr->count << ",";
            break;
        }
        case Backend::IL::TypeKind::Texture: {
            auto tex = type->As<Backend::IL::TextureType>();
            out.Line() << "\"Dimension\": " << static_cast<uint32_t>(tex->dimension) << ",";

            if (tex->sampledType) {
                out.Line() << "\"SampledType\": " << tex->sampledType->id << ",";
            }

            out.Line() << "\"Format\": " << static_cast<uint32_t>(tex->format) << ",";
            out.Line() << "\"SamplerMode\": " << static_cast<uint32_t>(tex->samplerMode) << ",";
            out.Line() << "\"MultiSampled\": " << tex->multisampled << ",";
            break;
        }
        case Backend::IL::TypeKind::Buffer: {
            auto buf = type->As<Backend::IL::BufferType>();
            out.Line() << "\"TexelType\": " << static_cast<uint32_t>(buf->texelType) << ",";
            out.Line() << "\"SamplerMode\": " << static_cast<uint32_t>(buf->samplerMode) << ",";

            if (buf->elementType) {
                out.Line() << "\"ElementType\": " << buf->elementType->id << ",";
            }
            break;
        }
        case Backend::IL::TypeKind::Sampler: {
            break;
        }
        case Backend::IL::TypeKind::CBuffer: {
            break;
        }
        case Backend::IL::TypeKind::Function: {
            auto fn = type->As<Backend::IL::FunctionType>();
            out.Line() << "\"ReturnType\": " << fn->returnType->id << ",";

            out.Line() << "\"ParameterTypes\": ";
            out.Line() << "[";
            
            for (size_t i = 0; i < fn->parameterTypes.size(); i++) {                
                if (i != fn->parameterTypes.size() - 1) {
                    out.Line() << "\t" << fn->parameterTypes[i]->id << ",";
                } else {
                    out.Line() << "\t" << fn->parameterTypes[i]->id;
                }
            }
            out.Line() << "],";
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto str = type->As<Backend::IL::StructType>();

            out.Line() << "\"Members\": ";
            out.Line() << "[";
            
            for (size_t i = 0; i < str->memberTypes.size(); i++) {         
                if (i != str->memberTypes.size() - 1) {
                    out.Line() << "\t" << str->memberTypes[i]->id << ",";
                } else {
                    out.Line() << "\t" << str->memberTypes[i]->id;
                }
            }

            out.Line() << "],";
            break;
        }
        case Backend::IL::TypeKind::Unexposed: {
            break;
        }
    }
    
    out.Line() << "\"ID\": " << type->id << ",";
    out.Line() << "\"Kind\": " << static_cast<uint32_t>(type->kind);
}

struct SymbolicContext {
    /// Symbolic constants mappings
    std::unordered_map<const IL::Constant*, int32_t> mappings;
};

/// Get or allocate a constant id
static int32_t GetConstantId(const Backend::IL::Constant *constant, SymbolicContext& context) {
    // Non-symbolic just use their id
    if (!constant->IsSymbolic()) {
        return constant->id;
    }

    // Already allocated?
    auto it = context.mappings.find(constant);
    if (it != context.mappings.end()) {
        return it->second;
    }

    // Symbolics are represented with negative values
    const int32_t id = -static_cast<int32_t>(context.mappings.size());
    return context.mappings[constant] = id;
}

void PrettyPrintJson(const Backend::IL::Constant *constant, SymbolicContext& context, IL::PrettyPrintContext out) {
    switch (constant->kind) {
        default:
            break;
        case Backend::IL::ConstantKind::Bool: {
            auto _bool = constant->As<Backend::IL::BoolConstant>();
            out.Line() << "\"Value\": " << _bool->value << ",";
            break;
        }
        case Backend::IL::ConstantKind::Int: {
            auto _int = constant->As<Backend::IL::IntConstant>();
            out.Line() << "\"Value\": " << _int->value << ",";
            break;
        }
        case Backend::IL::ConstantKind::FP: {
            auto fp = constant->As<Backend::IL::FPConstant>();
            out.Line() << "\"Value\": " << fp->value << ",";
            break;
        }
        case Backend::IL::ConstantKind::Array: {
            auto str = constant->As<Backend::IL::ArrayConstant>();
            
            out.Line() << "\"Elements\": ";
            out.Line() << "[";
            
            for (size_t i = 0; i < str->elements.size(); i++) {
                if (i != str->elements.size() - 1) {
                    out.Line() << "\t" << GetConstantId(str->elements[i], context) << ",";
                } else {
                    out.Line() << "\t" << GetConstantId(str->elements[i], context);
                }
            }

            out.Line() << "],";
            break;
        }
        case Backend::IL::ConstantKind::Struct: {
            auto str = constant->As<Backend::IL::StructConstant>();
            
            out.Line() << "\"Members\": ";
            out.Line() << "[";
            
            for (size_t i = 0; i < str->members.size(); i++) {
                if (i != str->members.size() - 1) {
                    out.Line() << "\t" << GetConstantId(str->members[i], context) << ",";
                } else {
                    out.Line() << "\t" << GetConstantId(str->members[i], context);
                }
            }

            out.Line() << "],";
            break;
        }
    }

    out.Line() << "\"ID\": " << GetConstantId(constant, context) << ",";
    out.Line() << "\"Kind\": " << static_cast<uint32_t>(constant->kind) << ",";
    out.Line() << "\"Type\": " << constant->type->id;
}

void PrettyPrintJson(const Backend::IL::Variable *variable, IL::PrettyPrintContext out) {
    out.Line() << "\"ID\": " << variable->id << ",";
    out.Line() << "\"AddressSpace\": " << static_cast<uint32_t>(variable->addressSpace) << ",";
    out.Line() << "\"Type\": " << variable->type->id;
}

void PrettyPrintJson(const IL::Program& program, const Backend::IL::Instruction* instr, IL::PrettyPrintContext out) {    
    switch(instr->opCode) {
        default:
            ASSERT(false, "Unknown instruction");
        break;
        case IL::OpCode::None: {
            break;
        }
        case IL::OpCode::Unexposed: {
            auto unexposed = instr->As<IL::UnexposedInstruction>();
            if (unexposed->symbol) {
                out.Line() << "\"Symbol\": \"" << unexposed->symbol << "\",";
            } else {
                out.Line() << "\"UnexposedOp\": \"" << unexposed->backendOpCode << "\",";
            }
            break;
        }
        case IL::OpCode::Literal: {
            auto literal = instr->As<IL::LiteralInstruction>();
            out.Line() << "\"Type\": " << static_cast<uint32_t>(literal->type) << ",";
            out.Line() << "\"BitWidth\": " << static_cast<uint32_t>(literal->bitWidth) << ",";
            out.Line() << "\"Signed\": " << static_cast<uint32_t>(literal->signedness) << ",";

            switch (literal->type) {
                case IL::LiteralType::None:
                    break;
                case IL::LiteralType::Int:
                    out.Line() << "\"Value\": " << literal->value.integral << ",";
                break;
                case IL::LiteralType::FP:
                    out.Line() << "\"Value\": " << literal->value.fp << ",";
                break;
            }
            break;
        }
        case IL::OpCode::Add: {
            auto add = instr->As<IL::AddInstruction>();
            out.Line() << "\"LHS\": " << add->lhs << ",";
            out.Line() << "\"RHS\": " << add->rhs << ",";
            break;
        }
        case IL::OpCode::Load: {
            auto load = instr->As<IL::LoadInstruction>();
            out.Line() << "\"Address\": " << load->address << ",";
            break;
        }
        case IL::OpCode::Store: {
            auto store = instr->As<IL::StoreInstruction>();
            out.Line() << "\"Address\": " << store->address << ",";
            out.Line() << "\"Value\": " << store->value << ",";
            break;
        }
        case IL::OpCode::SampleTexture: {
            auto load = instr->As<IL::SampleTextureInstruction>();
            out.Line() << "\"SampleMode\": " << static_cast<uint32_t>(load->sampleMode) << ",";
            out.Line() << "\"Texture\": " << load->texture << ",";

            if (load->sampler != IL::InvalidID) {
                out.Line() << "\"Sampler\": " << load->sampler << ",";
            }

            out.Line() << "\"Coordinate\": " << load->coordinate << ",";

            if (load->reference != IL::InvalidID) {
                out.Line() << "\"Reference\": " << load->reference << ",";
            }

            if (load->lod != IL::InvalidID) {
                out.Line() << "\"LOD\": " << load->lod << ",";
            }

            if (load->bias != IL::InvalidID) {
                out.Line() << "\"Bias\": " << load->bias << ",";
            }

            if (load->ddx != IL::InvalidID) {
                out.Line() << "\"DDX\": " << load->ddx << ",";
                out.Line() << "\"DDY\": " << load->ddy << ",";
            }
            break;
        }
        case IL::OpCode::LoadTexture: {
            auto load = instr->As<IL::LoadTextureInstruction>();
            out.Line() << "\"Texture\": " << load->texture << ",";
            out.Line() << "\"Index\": " << load->index << ",";

            if (load->offset != IL::InvalidID) {
                out.Line() << "\"Offset\": " << load->offset << ",";
            }
            if (load->mip != IL::InvalidID) {
                out.Line() << "\"Mip\": " << load->mip << ",";
            }
            break;
        }
        case IL::OpCode::StoreBuffer: {
            auto store = instr->As<IL::StoreBufferInstruction>();
            out.Line() << "\"Buffer\": " << store->buffer << ",";
            out.Line() << "\"Index\": " << store->index << ",";
            out.Line() << "\"Value\": " << store->value << ",";
            out.Line() << "\"ComponentMask\": " << store->mask.value << ",";
            break;
        }
        case IL::OpCode::StoreOutput: {
            auto store = instr->As<IL::StoreOutputInstruction>();
            out.Line() << "\"Index\": " << store->index << ",";
            out.Line() << "\"Row\": " << store->row << ",";
            out.Line() << "\"Column\": " << store->column << ",";
            out.Line() << "\"Value\": " << store->value << ",";
            break;
        }
        case IL::OpCode::IsInf: {
            auto isInf = instr->As<IL::IsInfInstruction>();
            out.Line() << "\"Value\": " << isInf->value << ",";
            break;
        }
        case IL::OpCode::IsNaN: {
            auto isNaN = instr->As<IL::IsNaNInstruction>();
            out.Line() << "\"Value\": " << isNaN->value << ",";
            break;
        }
        case IL::OpCode::Sub: {
            auto sub = instr->As<IL::SubInstruction>();
            out.Line() << "\"LHS\": " << sub->lhs << ",";
            out.Line() << "\"RHS\": " << sub->rhs << ",";
            break;
        }
        case IL::OpCode::Div: {
            auto sub = instr->As<IL::DivInstruction>();
            out.Line() << "\"LHS\": " << sub->lhs << ",";
            out.Line() << "\"RHS\": " << sub->rhs << ",";
            break;
        }
        case IL::OpCode::Mul: {
            auto mul = instr->As<IL::MulInstruction>();
            out.Line() << "\"LHS\": " << mul->lhs << ",";
            out.Line() << "\"RHS\": " << mul->rhs << ",";
            break;
        }
        case IL::OpCode::Or: {
            auto _or = instr->As<IL::OrInstruction>();
            out.Line() << "\"LHS\": " << _or->lhs << ",";
            out.Line() << "\"RHS\": " << _or->rhs << ",";
            break;
        }
        case IL::OpCode::And: {
            auto _and = instr->As<IL::AndInstruction>();
            out.Line() << "\"LHS\": " << _and->lhs << ",";
            out.Line() << "\"RHS\": " << _and->rhs << ",";
            break;
        }
        case IL::OpCode::Equal: {
            auto equal = instr->As<IL::EqualInstruction>();
            out.Line() << "\"LHS\": " << equal->lhs << ",";
            out.Line() << "\"RHS\": " << equal->rhs << ",";
            break;
        }
        case IL::OpCode::NotEqual: {
            auto notEqual = instr->As<IL::NotEqualInstruction>();
            out.Line() << "\"LHS\": " << notEqual->lhs << ",";
            out.Line() << "\"RHS\": " << notEqual->rhs << ",";
            break;
        }
        case IL::OpCode::LessThan: {
            auto lessThan = instr->As<IL::LessThanInstruction>();
            out.Line() << "\"LHS\": " << lessThan->lhs << ",";
            out.Line() << "\"RHS\": " << lessThan->rhs << ",";
            break;
        }
        case IL::OpCode::LessThanEqual: {
            auto lessThanEqual = instr->As<IL::LessThanEqualInstruction>();
            out.Line() << "\"LHS\": " << lessThanEqual->lhs << ",";
            out.Line() << "\"RHS\": " << lessThanEqual->rhs << ",";
            break;
        }
        case IL::OpCode::GreaterThan: {
            auto greaterThan = instr->As<IL::GreaterThanInstruction>();
            out.Line() << "\"LHS\": " << greaterThan->lhs << ",";
            out.Line() << "\"RHS\": " << greaterThan->rhs << ",";
            break;
        }
        case IL::OpCode::GreaterThanEqual: {
            auto greaterThanEqual = instr->As<IL::GreaterThanEqualInstruction>();
            out.Line() << "\"LHS\": " << greaterThanEqual->lhs << ",";
            out.Line() << "\"RHS\": " << greaterThanEqual->rhs << ",";
            break;
        }
        case IL::OpCode::AddressChain: {
            auto addressChain = instr->As<IL::AddressChainInstruction>();
            out.Line() << "\"Composite\": " << addressChain->composite << ",";

            out.Line() << "\"Chains\": ";
            out.Line() << "[";
            
            for (uint32_t i = 0; i < static_cast<uint32_t>(addressChain->chains.count); i++) {         
                if (i != addressChain->chains.count - 1) {
                    out.Line() << "\t" << addressChain->chains[i].index << ",";
                } else {
                    out.Line() << "\t" << addressChain->chains[i].index;
                }
            }

            out.Line() << "],";
            break;
        }
        case IL::OpCode::Extract: {
            auto extract = instr->As<IL::ExtractInstruction>();
            out.Line() << "\"Composite\": " << extract->composite << ",";
            out.Line() << "\"Index\": " << extract->index << ",";
            break;
        }
        case IL::OpCode::Insert: {
            auto insert = instr->As<IL::InsertInstruction>();
            out.Line() << "\"Composite\": " << insert->composite << ",";
            out.Line() << "\"Value\": " << insert->value << ",";
            break;
        }
        case IL::OpCode::Select: {
            auto select = instr->As<IL::SelectInstruction>();
            out.Line() << "\"Condition\": " << select->condition << ",";
            out.Line() << "\"Pass\": " << select->pass << ",";
            out.Line() << "\"Fail\": " << select->fail << ",";
            break;
        }
        case IL::OpCode::Branch: {
            auto branch = instr->As<IL::BranchInstruction>();
            out.Line() << "\"Branch\": " << branch->branch << ",";
            
            // Optional control flow
            if (branch->controlFlow.merge != IL::InvalidID) {
                out.Line() << "\"Merge\": " << branch->controlFlow.merge << ",";

                if (branch->controlFlow._continue != IL::InvalidID) {
                    out.Line() << "\"Continue\": " << branch->controlFlow._continue << ",";
                }
            }
            break;
        }
        case IL::OpCode::BranchConditional: {
            auto branch = instr->As<IL::BranchConditionalInstruction>();
            out.Line() << "\"Condition\": " << branch->cond << ",";
            out.Line() << "\"Pass\": " << branch->pass << ",";
            out.Line() << "\"Fail\": " << branch->fail << ",";

            // Optional control flow
            if (branch->controlFlow.merge != IL::InvalidID) {
                out.Line() << "\"Merge\": " << branch->controlFlow.merge << ",";

                if (branch->controlFlow._continue != IL::InvalidID) {
                    out.Line() << "\"Continue\": " << branch->controlFlow._continue << ",";
                }
            }

            break;
        }
        case IL::OpCode::Switch: {
            auto _switch = instr->As<IL::SwitchInstruction>();
            out.Line() << "\"Value\": " << _switch->value << ",";
            out.Line() << "\"Default\": " << _switch->_default << ",";
            
            // Optional control flow
            if (_switch->controlFlow.merge != IL::InvalidID) {
                out.Line() << "\"Merge\": " << _switch->controlFlow.merge << ",";

                if (_switch->controlFlow._continue != IL::InvalidID) {
                    out.Line() << "\"Continue\": " << _switch->controlFlow._continue << ",";
                }
            }

            out.Line() << "\"Cases\": ";
            out.Line() << "[";
            
            for (uint32_t i = 0; i < _switch->cases.count; i++) {    
                const IL::SwitchCase &_case = _switch->cases[i];
                
                out.Line() << "{";
                out.Line() << "\"Branch\": " << _case.branch << ",";
                out.Line() << "\"Literal\": " << _case.literal;
                out.Line() << "}";

                if (i != _switch->cases.count - 1) {
                    out.stream << ",";
                }
            }

            out.Line() << "],";
            break;
        }
        case IL::OpCode::Phi: {
            auto phi = instr->As<IL::PhiInstruction>();
            
            out.Line() << "\"Values\": ";
            out.Line() << "[";
            
            for (uint32_t i = 0; i < phi->values.count; i++) {    
                const IL::PhiValue &value = phi->values[i];
                
                out.Line() << "{";
                out.Line() << "\t\"Branch\": " << value.branch << ",";
                out.Line() << "\t\"Value\": " << value.value;
                out.Line() << "}";

                if (i != phi->values.count - 1) {
                    out.stream << ",";
                }
            }

            out.Line() << "],";
            break;
        }
        case IL::OpCode::BitOr: {
            auto bitOr = instr->As<IL::BitOrInstruction>();
            out.Line() << "\"LHS\": " << bitOr->lhs << ",";
            out.Line() << "\"RHS\": " << bitOr->rhs << ",";
            break;
        }
        case IL::OpCode::BitAnd: {
            auto bitAnd = instr->As<IL::BitAndInstruction>();
            out.Line() << "\"LHS\": " << bitAnd->lhs << ",";
            out.Line() << "\"RHS\": " << bitAnd->rhs << ",";
            break;
        }
        case IL::OpCode::BitShiftLeft: {
            auto bitShiftLeft = instr->As<IL::BitShiftLeftInstruction>();
            out.Line() << "\"Value\": " << bitShiftLeft->value << ",";
            out.Line() << "\"Shift\": " << bitShiftLeft->shift << ",";
            break;
        }
        case IL::OpCode::BitShiftRight: {
            auto bitShiftRight = instr->As<IL::BitShiftRightInstruction>();
            out.Line() << "\"Value\": " << bitShiftRight->value << ",";
            out.Line() << "\"Shift\": " << bitShiftRight->shift << ",";
            break;
        }
        case IL::OpCode::Export: {
            auto _export = instr->As<IL::ExportInstruction>();
            out.Line() << "\"ExportID\": " << _export->exportID << ",";

            out.Line() << "\"Values\": ";
            out.Line() << "[";
            
            for (uint32_t i = 0; i < _export->values.count; i++) {    
                out.Line() << "\t" << _export->values[i];

                if (i != _export->values.count - 1) {
                    out.stream << ",";
                }
            }

            out.Line() << "],";
            break;
        }
        case IL::OpCode::Alloca: {
            auto _alloca = instr->As<IL::AllocaInstruction>();
            out.Line() << "\"Type\": " << program.GetTypeMap().GetType(_alloca->result)->id << ",";
            break;
        }
        case IL::OpCode::StoreTexture: {
            auto store = instr->As<IL::StoreTextureInstruction>();
            out.Line() << "\"Texture\": " << store->texture << ",";
            out.Line() << "\"Index\": " << store->index << ",";
            out.Line() << "\"Texel\": " << store->texel << ",";
            out.Line() << "\"ComponentMask\": " << store->mask.value << ",";
            break;
        }
        case IL::OpCode::Any: {
            auto _or = instr->As<IL::AnyInstruction>();
            out.Line() << "\"Value\": " << _or->value << ",";
            break;
        }
        case IL::OpCode::All: {
            auto load = instr->As<IL::AllInstruction>();
            out.Line() << "\"Value\": " << load->value << ",";
            break;
        }
        case IL::OpCode::LoadBuffer: {
            auto load = instr->As<IL::LoadBufferInstruction>();
            out.Line() << "\"Buffer\": " << load->buffer << ",";
            out.Line() << "\"Index\": " << load->index << ",";

            if (load->offset != IL::InvalidID) {
                out.Line() << "\"Offset\": " << load->offset << ",";
            }
            break;
        }
        case IL::OpCode::ResourceSize: {
            auto load = instr->As<IL::ResourceSizeInstruction>();
            out.Line() << "\"Resource\": " << load->resource << ",";
            break;
        }
        case IL::OpCode::ResourceToken: {
            auto load = instr->As<IL::ResourceTokenInstruction>();
            out.Line() << "\"Resource\": " << load->resource << ",";
            break;
        }
        case IL::OpCode::Rem: {
            auto rem = instr->As<IL::RemInstruction>();
            out.Line() << "\"LHS\": " << rem->lhs << ",";
            out.Line() << "\"RHS\": " << rem->rhs << ",";
            break;
        }
        case IL::OpCode::Trunc: {
            auto trunc = instr->As<IL::TruncInstruction>();
            out.Line() << "\"Value\": " << trunc->value << ",";
            break;
        }
        case IL::OpCode::Return: {
            auto ret = instr->As<IL::ReturnInstruction>();

            if (ret->value != IL::InvalidID) {
                out.Line() << "\"Value\": " << ret->value << ",";
            }
            break;
        }
        case IL::OpCode::BitXOr: {
            auto _xor = instr->As<IL::BitXOrInstruction>();
            out.Line() << "\"LHS\": " << _xor->lhs << ",";
            out.Line() << "\"RHS\": " << _xor->rhs << ",";
            break;
        }
        case IL::OpCode::FloatToInt: {
            auto ftoi = instr->As<IL::FloatToIntInstruction>();
            out.Line() << "\"Value\": " << ftoi->value << ",";
            break;
        }
        case IL::OpCode::IntToFloat: {
            auto itof = instr->As<IL::IntToFloatInstruction>();
            out.Line() << "\"Value\": " << itof->value << ",";
            break;
        }
        case IL::OpCode::BitCast: {
            auto cast = instr->As<IL::BitCastInstruction>();
            out.Line() << "\"Value\": " << cast->value << ",";
            break;
        }
        case IL::OpCode::AtomicOr: {
            auto atomic = instr->As<IL::AtomicOrInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicXOr: {
            auto atomic = instr->As<IL::AtomicXOrInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicAnd: {
            auto atomic = instr->As<IL::AtomicAndInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicAdd: {
            auto atomic = instr->As<IL::AtomicAddInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicMin: {
            auto atomic = instr->As<IL::AtomicMinInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicMax: {
            auto atomic = instr->As<IL::AtomicMinInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicExchange: {
            auto atomic = instr->As<IL::AtomicExchangeInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::AtomicCompareExchange: {
            auto atomic = instr->As<IL::AtomicCompareExchangeInstruction>();
            out.Line() << "\"Address\": " << atomic->address << ",";
            out.Line() << "\"Value\": " << atomic->value << ",";
            out.Line() << "\"Comparator\": " << atomic->comparator << ",";
            break;
        }
        case IL::OpCode::WaveAnyTrue: {
            auto atomic = instr->As<IL::WaveAnyTrueInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveAllTrue: {
            auto atomic = instr->As<IL::WaveAllTrueInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveBallot: {
            auto atomic = instr->As<IL::WaveBallotInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveRead: {
            auto atomic = instr->As<IL::WaveReadInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            out.Line() << "\"Lane\": " << atomic->lane << ",";
            break;
        }
        case IL::OpCode::WaveReadFirst: {
            auto atomic = instr->As<IL::WaveReadFirstInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveAllEqual: {
            auto atomic = instr->As<IL::WaveAllEqualInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveBitAnd: {
            auto atomic = instr->As<IL::WaveBitAndInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveBitOr: {
            auto atomic = instr->As<IL::WaveBitOrInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveBitXOr: {
            auto atomic = instr->As<IL::WaveBitXOrInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveCountBits: {
            auto atomic = instr->As<IL::WaveCountBitsInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveMax: {
            auto atomic = instr->As<IL::WaveMaxInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveMin: {
            auto atomic = instr->As<IL::WaveMinInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveProduct: {
            auto atomic = instr->As<IL::WaveProductInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WaveSum: {
            auto atomic = instr->As<IL::WaveSumInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WavePrefixCountBits: {
            auto atomic = instr->As<IL::WavePrefixCountBitsInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WavePrefixProduct: {
            auto atomic = instr->As<IL::WavePrefixProductInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
        case IL::OpCode::WavePrefixSum: {
            auto atomic = instr->As<IL::WavePrefixSumInstruction>();
            out.Line() << "\"Value\": " << atomic->value << ",";
            break;
        }
    }
    
    out.Line() << "\"OpCode\": " << static_cast<uint32_t>(instr->opCode) << ",";
    
    if (const Backend::IL::Type* type = program.GetTypeMap().GetType(instr->result)) {
        out.Line() << "\"Type\": " << type->id << ",";
    }
    
    out.Line() << "\"ID\": " << instr->result;
}

void PrettyPrintJson(const Backend::IL::Program& program, const Backend::IL::BasicBlock* basicBlock, IL::PrettyPrintContext out) {
    out.Line() << "\"ID\": " << basicBlock->GetID() << ",";
    
    out.Line() << "\"Instructions\": ";
    out.Line() << "[";

    for (IL::BasicBlock::ConstIterator it = basicBlock->begin(); it != basicBlock->end(); ++it) {
        out.Line() << "{";
        PrettyPrintJson(program, *it, out.Tab());
        out.Line() << "}";

        if (std::next(it) != basicBlock->end()) {
            out.stream << ",";
        }
    }
    
    out.Line() << "]";
}

void PrettyPrintJson(const Backend::IL::Program& program, const Backend::IL::Function* function, IL::PrettyPrintContext out) {
    out.Line() << "\"ID\": " << function->GetID() << ",";
    out.Line() << "\"Type\": " << function->GetFunctionType()->id << ",";

    out.Line() << "\"Parameters\": ";
    out.Line() << "[";

    const IL::VariableList& variables = function->GetParameters();
    
    for (const Backend::IL::Variable* variable : variables) {
        out.Line() << "\t" << variable->id;
        
        if (variable != *--variables.end()) {
            out.stream << ",";
        }
    }
    
    out.Line() << "],";
    
    const IL::BasicBlockList& blocks = function->GetBasicBlocks();

    out.Line() << "\"BasicBlocks\": ";
    out.Line() << "[";

    for (IL::BasicBlock* block : blocks) {
        out.Line() << "{";
        PrettyPrintJson(program, block, out.Tab());
        out.Line() << "}";
        
        if (block != *--blocks.end()) {
            out.stream << ",";
        }
    }
    
    out.Line() << "]";
}

void IL::PrettyPrintProgramJson(const Program& program, PrettyPrintContext out) {
    // Begin document
    out.Line() << "{";

    // Get entrypoint
    const Function *entryPoint = program.GetEntryPoint();

    // Metadata
    out.Line() << "\"AllocatedIdentifiers\": " << program.GetIdentifierMap().GetMaxID() << ",";
    out.Line() << "\"EntryPoint\": " << (entryPoint ? entryPoint->GetID() : InvalidID)  << ",";
    out.Line() << "\"GUID\": " << program.GetShaderGUID() << ",";

    // Emit types
    {        
        out.Line() << "\"Types\":";
        out.Line() << "[";

        PrettyPrintContext ctx = out.Tab();
        
        for (const Backend::IL::Type* type : program.GetTypeMap()) {
            ctx.Line() << "{";
            PrettyPrintJson(type, ctx.Tab());
            ctx.Line() << "}";

            if (type != *--program.GetTypeMap().end()) {
                ctx.stream << ",";
            }
        }
    
        ctx.Line() << "],";
    }

    // Emit constants
    {
        out.Line() << "\"Constants\":";
        out.Line() << "[";

        PrettyPrintContext ctx = out.Tab();

        SymbolicContext symbolicCtx;

        for (const Backend::IL::Constant* constant : program.GetConstants()) {
            ctx.Line() << "{";
            PrettyPrintJson(constant, symbolicCtx, ctx.Tab());
            ctx.Line() << "}";

            if (constant != *--program.GetConstants().end()) {
                ctx.stream << ",";
            }
        }
    
        out.Line() << "],";
    }

    // Emit variables
    {
        out.Line() << "\"Variables\":";
        out.Line() << "[";

        PrettyPrintContext ctx = out.Tab();

        for (const Backend::IL::Variable* variable : program.GetVariableList()) {
            ctx.Line() << "{";
            PrettyPrintJson(variable, ctx.Tab());
            ctx.Line() << "}";

            if (variable != *--program.GetVariableList().end()) {
                ctx.stream << ",";
            }
        }
    
        out.Line() << "],";
    }

    // Emit functions
    {
        out.Line() << "\"Functions\":";
        out.Line() << "[";

        PrettyPrintContext ctx = out.Tab();

        for (const Backend::IL::Function* function : program.GetFunctionList()) {
            ctx.Line() << "{";
            PrettyPrintJson(program, function, ctx.Tab());
            ctx.Line() << "}";

            if (function != *--program.GetFunctionList().end()) {
                ctx.stream << ",";
            }
        }
    
        out.Line() << "]";
    }

    // Close document
    out.Line() << "}";
}
