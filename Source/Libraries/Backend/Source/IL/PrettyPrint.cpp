#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/Program.h>
#include <Backend/IL/Function.h>
#include <Backend/IL/BasicBlock.h>

void IL::PrettyPrint(const Program &program, IL::PrettyPrintContext out) {
    out.Line() << "Program\n";
    out.Line() << "Bound     : " << program.GetIdentifierMap().GetMaxID() << "\n";
    out.Line() << "Functions : " << program.GetFunctionCount() << "\n";
    out.Line() << "\n";

    // Print all functions
    for (const IL::Function &fn: program) {
        PrettyPrint(fn, out);
    }
}

void IL::PrettyPrint(const Function &function, IL::PrettyPrintContext out) {
    out.Line() << "%" << function.GetID() << " = Function\n";

    // Print all basic blocks
    for (const IL::BasicBlock &bb: function) {
        PrettyPrint(bb, out.Tab());
    }
}

void IL::PrettyPrint(const BasicBlock &basicBlock, IL::PrettyPrintContext out) {
    out.Line() << "%" << basicBlock.GetID() << " = BasicBlock\n";

    // Print all instructions
    for (const IL::Instruction *instr: basicBlock) {
        PrettyPrint(instr, out.Tab());
    }
}

void IL::PrettyPrint(const Instruction *instr, IL::PrettyPrintContext out) {
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
            line << "Unexposed op:" << unexposed->backendOpCode;
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
        case OpCode::Sub: {
            auto sub = instr->As<IL::SubInstruction>();
            line << "Sub %" << sub->lhs << " %" << sub->rhs;
            break;
        }
        case OpCode::Div: {
            auto sub = instr->As<IL::SubInstruction>();
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
        case OpCode::Branch: {
            auto branch = instr->As<IL::BranchInstruction>();
            line << "Branch %" << branch->branch;
            break;
        }
        case OpCode::BranchConditional: {
            auto branch = instr->As<IL::BranchConditionalInstruction>();
            line << "BranchConditional cond:%" << branch->cond << " pass:%" << branch->pass << " fail:%" << branch->fail;
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
            line << "Export value:%" << _export->value << " exportID:" << _export->exportID;
            break;
        }
        case OpCode::Alloca: {
            auto _alloca = instr->As<IL::AllocaInstruction>();
            line << "Alloca %" << _alloca->type;
            break;
        }
        case OpCode::StoreTexture: {
            auto load = instr->As<IL::StoreTextureInstruction>();
            line << "StoreTexture texture:%" << load->texture << " index:%" << load->index << " texel:%" << load->texel;
            break;
        }
        case OpCode::SourceAssociation: {
            auto load = instr->As<IL::SourceAssociationInstruction>();
            line << "SourceAssociation file:" << load->file << " line:" << load->line << " column:" << load->column;
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
            break;
        }
        case OpCode::ResourceSize: {
            auto load = instr->As<IL::ResourceSizeInstruction>();
            line << "ResourceSize %" << load->resource;
            break;
        }
    }

    if (instr->source.IsValid()) {
        line << " [" << instr->source.codeOffset << "]";
    }

    line << "\n";
}
