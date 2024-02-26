#pragma once

// Backend
#include <Backend/IL/Constant/ConstantFoldingCommon.h>

// Std
#include <bit>

namespace Backend::IL {
    /// Helper, integral type check without booleans
    template<typename T>
    constexpr bool IsNumericV = std::is_integral_v<T> && !std::is_same_v<T, bool>;

    /// Decay a given constant value
    template<typename T>
    decltype(T::value) DecayConstant(const Constant* constant) {
        return constant->As<T>()->value;
    }

    /// Decay a constant range
    template<typename T, typename... C>
    auto DecayConstantRange(const std::tuple<C...>& constants) {
        return std::apply([](auto& ...cx) { return std::make_tuple(DecayConstant<T>(cx)...); }, constants);
    }

    /// Fold a numeric set to a type, decays
    template<typename T, typename... C, typename F>
    const Constant* FoldNumericConstantsToType(Program& program, const Type* type, const std::tuple<C...>& cx, F&& functor) {
        ConstantMap& constants = program.GetConstants();

        // Get first constant
        const Constant* ca = std::get<0>(cx);

        // Handle kind
        decltype(T::value) value;
        switch (ca->kind) {
            case ConstantKind::None:
            case ConstantKind::Undef:
            case ConstantKind::Struct:
            case ConstantKind::Vector:
            case ConstantKind::Array:
            case ConstantKind::Null:
            case ConstantKind::Unexposed: {
                return nullptr;
            }
            case ConstantKind::Bool: {
                value = std::apply(functor, DecayConstantRange<BoolConstant>(cx));
                break;
            }
            case ConstantKind::Int: {
                value = std::apply(functor, DecayConstantRange<IntConstant>(cx));
                break;
            }
            case ConstantKind::FP: {
                value = std::apply(functor, DecayConstantRange<FPConstant>(cx));
                break;
            }
        }

        // Create constant
        return constants.FindConstantOrAdd(
            type->As<typename T::Type>(),
            T{.value = value}
        );
    }

    /// Fold a set of numeric constants, decays
    template<typename... C, typename F>
    const Constant* FoldNumericConstants(Program& program, const std::tuple<C...>& cx, F&& functor) {
        ConstantMap& constants = program.GetConstants();

        // Get first constant
        const Constant* ca = std::get<0>(cx);

        // Handle kind
        switch (ca->kind) {
            default: {
                ASSERT(false, "Invalid path, missing mapping");
                return nullptr;
            }
            case ConstantKind::None:
            case ConstantKind::Undef:
            case ConstantKind::Struct:
            case ConstantKind::Vector:
            case ConstantKind::Array:
            case ConstantKind::Null:
            case ConstantKind::Unexposed: {
                return nullptr;
            }
            case ConstantKind::Bool: {
                return constants.FindConstantOrAdd(
                    ca->type->As<BoolType>(),
                    BoolConstant{.value = static_cast<bool>(std::apply(functor, DecayConstantRange<BoolConstant>(cx)))}
                );
            }
            case ConstantKind::Int: {
                return constants.FindConstantOrAdd(
                    ca->type->As<IntType>(),
                    IntConstant{.value = static_cast<int64_t>(std::apply(functor, DecayConstantRange<IntConstant>(cx)))}
                );
            }
            case ConstantKind::FP: {
                return constants.FindConstantOrAdd(
                    ca->type->As<FPType>(),
                    FPConstant{.value = static_cast<double>(std::apply(functor, DecayConstantRange<FPConstant>(cx)))}
                );
            }
        }
    }

    /// Fold an instruction
    /// Must be known foldable with immediates
    template<typename F>
    const Constant* FoldConstantInstruction(Program& program, const Instruction* instr, F&& map) {
        ConstantMap& constants = program.GetConstants();
        TypeMap&     types     = program.GetTypeMap();
        
        switch (instr->opCode) {
            default: {
                ASSERT(false, "Invalid path, missing mapping");
                return nullptr;
            }
            
            /* See ConstantFoldingCommon.h for general expectations */
            case OpCode::None:
            case OpCode::Branch:
            case OpCode::BranchConditional:
            case OpCode::Switch:
            case OpCode::Phi:
            case OpCode::Return:
            case OpCode::Call:
            case OpCode::AtomicOr:
            case OpCode::AtomicXOr:
            case OpCode::AtomicAnd:
            case OpCode::AtomicAdd:
            case OpCode::AtomicMin:
            case OpCode::AtomicMax:
            case OpCode::AtomicExchange:
            case OpCode::AtomicCompareExchange:
            case OpCode::StoreOutput:
            case OpCode::SampleTexture:
            case OpCode::StoreTexture:
            case OpCode::LoadTexture:
            case OpCode::StoreBuffer:
            case OpCode::LoadBuffer:
            case OpCode::WaveAnyTrue:
            case OpCode::WaveAllTrue:
            case OpCode::WaveBallot:
            case OpCode::WaveRead:
            case OpCode::WaveReadFirst:
            case OpCode::WaveAllEqual:
            case OpCode::WaveBitAnd:
            case OpCode::WaveBitOr:
            case OpCode::WaveBitXOr:
            case OpCode::WaveCountBits:
            case OpCode::WaveMax:
            case OpCode::WaveMin:
            case OpCode::WaveProduct:
            case OpCode::WaveSum:
            case OpCode::WavePrefixCountBits:
            case OpCode::WavePrefixProduct:
            case OpCode::WavePrefixSum:
            case OpCode::ResourceToken:
            case OpCode::ResourceSize:
            case OpCode::Export:
            case OpCode::Alloca:
            case OpCode::Load:
            case OpCode::Store: {
                return nullptr;
            }
            case OpCode::Literal: {
                auto _instr = instr->As<LiteralInstruction>();
                switch (_instr->type) {
                    default: {
                        ASSERT(false, "Invalid literal instruction");
                        return nullptr;
                    }
                    case IL::LiteralType::Int: {
                        return constants.FindConstantOrAdd(
                            types.FindTypeOrAdd(IntType{.bitWidth=_instr->bitWidth, .signedness=_instr->signedness}),
                            IntConstant{.value = _instr->value.integral}
                        );
                    }
                    case IL::LiteralType::FP: {
                        return constants.FindConstantOrAdd(
                            types.FindTypeOrAdd(FPType{.bitWidth=_instr->bitWidth}),
                            FPConstant{.value = _instr->value.fp}
                        );
                    }
                }
            }
            case OpCode::Any: {
                auto _instr = instr->As<AnyInstruction>();

                const Constant* value = map(_instr->value);

                // TODO: Vector types
                switch (value->kind) {
                    default:
                        return nullptr;
                    case ConstantKind::Bool:
                        return value;
                }
            }
            case OpCode::All: {
                auto _instr = instr->As<AllInstruction>();

                const Constant* value = map(_instr->value);

                // TODO: Vector types
                switch (value->kind) {
                    default:
                        return nullptr;
                    case ConstantKind::Bool:
                        return value;
                }
            }
            case OpCode::Add: {
                auto _instr = instr->As<AddInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs + rhs;
                });
            }
            case OpCode::Sub: {
                auto _instr = instr->As<SubInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs - rhs;
                });
            }
            case OpCode::Div: {
                auto _instr = instr->As<DivInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) -> decltype(lhs) {
                    if (rhs == 0) {
                        return 0;
                    }
                    
                    if constexpr(!std::is_same_v<decltype(lhs), bool>) {
                        return lhs / rhs;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::Mul: {
                auto _instr = instr->As<MulInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs * rhs;
                });
            }
            case OpCode::Rem: {
                auto _instr = instr->As<RemInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) -> decltype(lhs) {
                    if (rhs == 0) {
                        return 0;
                    }
                    
                    if constexpr(std::is_floating_point_v<decltype(lhs)>) {
                        return std::fmod(lhs, rhs);
                    } else if constexpr(!std::is_same_v<decltype(lhs), bool>) {
                        return lhs % rhs;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::Trunc: {
                auto _instr = instr->As<TruncInstruction>();
                return map(_instr->value); // TODO: Placeholder
            }
            case OpCode::Or: {
                auto _instr = instr->As<OrInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs || rhs;
                });
            }
            case OpCode::And: {
                auto _instr = instr->As<AndInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs && rhs;
                });
            }
            case OpCode::Equal: {
                auto _instr = instr->As<EqualInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)),    [](auto lhs, auto rhs) {
                    return lhs == rhs;
                });
            }
            case OpCode::NotEqual: {
                auto _instr = instr->As<NotEqualInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs != rhs;
                });
            }
            case OpCode::LessThan: {
                auto _instr = instr->As<LessThanInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs < rhs;
                });
            }
            case OpCode::LessThanEqual: {
                auto _instr = instr->As<LessThanEqualInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs <= rhs;
                });
            }
            case OpCode::GreaterThan: {
                auto _instr = instr->As<GreaterThanInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs > rhs;
                });
            }
            case OpCode::GreaterThanEqual: {
                auto _instr = instr->As<GreaterThanEqualInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    return lhs >= rhs;
                });
            }
            case OpCode::IsInf: {
                auto _instr = instr->As<IsInfInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->value)), [](auto value) {
                    if constexpr(std::is_floating_point_v<decltype(value)>) {
                        return std::isinf(value);
                    } else {
                        return false;
                    }
                });
            }
            case OpCode::IsNaN: {
                auto _instr = instr->As<IsNaNInstruction>();
                return FoldNumericConstantsToType<BoolConstant>(program, types.GetType(_instr->result), std::make_tuple(map(_instr->value)), [](auto value) {
                    if constexpr(std::is_floating_point_v<decltype(value)>) {
                        return std::isnan(value);
                    } else {
                        return false;
                    }
                });
            }
            case OpCode::Select: {
                auto _instr = instr->As<SelectInstruction>();
                if (map(_instr->condition)->template As<BoolConstant>()->value) {
                    return map(_instr->pass);
                } else {
                    return map(_instr->fail);
                }
            }
            case OpCode::BitOr: {
                auto _instr = instr->As<BitOrInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    if constexpr(IsNumericV<decltype(lhs)>) {
                        return lhs | rhs;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::BitXOr:  {
                auto _instr = instr->As<BitXOrInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    if constexpr(IsNumericV<decltype(lhs)>) {
                        return lhs ^ rhs;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::BitAnd:  {
                auto _instr = instr->As<BitAndInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->lhs), map(_instr->rhs)), [](auto lhs, auto rhs) {
                    if constexpr(IsNumericV<decltype(lhs)>) {
                        return lhs & rhs;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::BitShiftLeft: {
                auto _instr = instr->As<BitShiftLeftInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->value), map(_instr->shift)), [](auto value, auto shift) {
                    if constexpr(IsNumericV<decltype(value)>) {
                        return value << shift;
                    } else {
                        return 0;
                    }
                });
            }
            case OpCode::BitShiftRight:  {
                auto _instr = instr->As<BitShiftRightInstruction>();
                return FoldNumericConstants(program, std::make_tuple(map(_instr->value), map(_instr->shift)), [](auto value, auto shift) {
                    if constexpr(IsNumericV<decltype(value)>) {
                         return value >> shift;
                     } else {
                         return 0;
                     }
                });
            }
            case OpCode::AddressChain: {
                auto _instr = instr->As<AddressChainInstruction>();

                const Constant* constant = map(_instr->composite);

                for (uint32_t i = 0; i < _instr->chains.count; i++) {
                    AddressChain chain = _instr->chains[i];

                    // Get index
                    const auto* index = map(chain.index)->template As<IntConstant>();

                    // Handle composite type
                    switch (constant->kind) {
                        case ConstantKind::None:
                        case ConstantKind::Undef:
                        case ConstantKind::Bool:
                        case ConstantKind::Int:
                        case ConstantKind::FP:
                        case ConstantKind::Unexposed:
                        case ConstantKind::Null:
                            return nullptr;
                        case ConstantKind::Array:
                            constant = constant->As<ArrayConstant>()->elements[index->value];
                            break;
                        case ConstantKind::Struct:
                            constant = constant->As<StructConstant>()->members[index->value];
                            break;
                        case ConstantKind::Vector:
                            constant = constant->As<VectorConstant>()->elements[index->value];
                            break;
                    }
                }

                return constant;
            }
            case OpCode::Extract: {
                auto _instr = instr->As<ExtractInstruction>();

                const Constant* constant = map(_instr->composite);
                const Constant* index    = map(_instr->index);

                // Handle composite type
                switch (constant->kind) {
                    case ConstantKind::None:
                    case ConstantKind::Undef:
                    case ConstantKind::Bool:
                    case ConstantKind::Int:
                    case ConstantKind::FP:
                    case ConstantKind::Unexposed:
                    case ConstantKind::Null:
                        return nullptr;
                    case ConstantKind::Array:
                        return constant->As<ArrayConstant>()->elements[index->As<IntConstant>()->value];
                    case ConstantKind::Struct:
                        return constant->As<StructConstant>()->members[index->As<IntConstant>()->value];
                    case ConstantKind::Vector:
                        return constant->As<VectorConstant>()->elements[index->As<IntConstant>()->value];
                }
            }
            case OpCode::Insert: {
                // TODO: Implement cfold vector insertion
                return nullptr;
            }
            case OpCode::FloatToInt: {
                auto _instr = instr->As<FloatToIntInstruction>();

                return constants.FindConstantOrAdd(
                    types.GetType(_instr->result)->As<IntType>(),
                    IntConstant{.value = static_cast<int64_t>(map(_instr->value)->template As<FPConstant>()->value)}
                );
            }
            case OpCode::IntToFloat: {
                auto _instr = instr->As<IntToFloatInstruction>();

                return constants.FindConstantOrAdd(
                    types.GetType(_instr->result)->As<FPType>(),
                    FPConstant{.value = static_cast<double>(map(_instr->value)->template As<IntConstant>()->value)}
                );
            }
            case OpCode::BitCast: {
                auto _instr = instr->As<BitCastInstruction>();

                const Constant* constant = map(_instr->value);

                const Type* targetType = types.GetType(_instr->result);

                switch (targetType->kind) {
                    case TypeKind::None:
                    case TypeKind::Void:
                    case TypeKind::Vector:
                    case TypeKind::Matrix:
                    case TypeKind::Pointer:
                    case TypeKind::Array:
                    case TypeKind::Texture:
                    case TypeKind::Buffer:
                    case TypeKind::Sampler:
                    case TypeKind::CBuffer:
                    case TypeKind::Function:
                    case TypeKind::Struct:
                    case TypeKind::Unexposed:
                    case TypeKind::Bool:
                        return nullptr;
                    case TypeKind::Int:{
                        return FoldNumericConstantsToType<IntConstant>(program, types.GetType(_instr->result), std::make_tuple(constant), [](auto value) {
                            if constexpr(!std::is_same_v<decltype(value), bool>) {
                                return std::bit_cast<int64_t>(value);
                            } else {
                                return 0;
                            }
                        });
                    }
                    case TypeKind::FP:{
                        return FoldNumericConstantsToType<FPConstant>(program, types.GetType(_instr->result), std::make_tuple(constant), [](auto value) {
                            if constexpr(!std::is_same_v<decltype(value), bool>) {
                                return std::bit_cast<double>(value);
                            } else {
                                return 0;
                            }
                        });
                    }
                }
            }

            /** We make a special exception for unexposed operations, assume foldable */
            case OpCode::Unexposed: {
                return constants.AddSymbolicConstant(
                    types.GetType(instr->result),
                    UnexposedConstant { }
                );
            }
        }
    }
}
