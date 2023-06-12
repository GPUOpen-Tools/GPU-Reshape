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

    // Print all functions
    for (const IL::Function *fn: program.GetFunctionList()) {
        if (*program.GetFunctionList().begin() != fn) {
            out.Line() << "\n";
        }
        
        PrettyPrint(*fn, out);
    }
}

void IL::PrettyPrint(const Function &function, IL::PrettyPrintContext out) {
    out.Line() << "%" << function.GetID() << " = Function\n";

    // Print all basic blocks
    for (const IL::BasicBlock *bb: function.GetBasicBlocks()) {
        PrettyPrint(*bb, out.Tab());
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
            if (unexposed->symbol) {
                line << "Unexposed '" << unexposed->symbol << "'";
            } else {
                line << "Unexposed op:" << unexposed->backendOpCode;
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
            line << "Alloca %" << _alloca->type;
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
            line << "Pointer pointee:%" << ptr->pointee->id << " space:";
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

        line << "%" << constant->id << " = type:%" << constant->type->id << " ";

        switch (constant->kind) {
            default: {
                line << "Unexposed";
                break;
            }
            case Backend::IL::ConstantKind::Bool: {
                auto _bool = constant->As<Backend::IL::BoolConstant>();
                line << "Bool ";
                if (_bool->value) {
                    line << "true";
                } else {
                    line << "false";
                }
                break;
            }
            case Backend::IL::ConstantKind::Int: {
                auto _int = constant->As<Backend::IL::IntConstant>();

                if (_int->type->As<Backend::IL::IntType>()->signedness) {
                    line << "Int";
                } else {
                    line << "UInt";
                }

                line << " " << _int->value;
                break;
            }
            case Backend::IL::ConstantKind::FP: {
                auto fp = constant->As<Backend::IL::FPConstant>();
                line << "FP";
                line << " " << fp->value;
                break;
            }
            case Backend::IL::ConstantKind::Undef: {
                PrettyPrint(constant->type, out);
                line << " Undef";
                break;
            }
        }

        line << "\n";
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
