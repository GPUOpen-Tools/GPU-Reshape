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
    }

    if (instr->source != IL::InvalidSource) {
        line << " [" << instr->source << "]";
    }

    line << "\n";
}
