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

#include <Backends/Vulkan/IL/DebugEmitter.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>

// Backend
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>

// Spirv
#include <spirv/unified1/NonSemanticShaderDebugInfo100.h>

struct DebugHandle {
    /// Assigned value
    IL::ID result;
    
    /// Optional, access indices for code values
    std::span<uint32_t> accessIndices;
};

DebugEmitter::DebugEmitter(DeviceDispatchTable* table) : table(table) {
    
}

static void CreateVariableValues(const Backend::IL::Type* type, SmallArena& arena, IL::DebugSingleValue& value) {
    value.type = type;
    value.name = "";
    
    switch (type->kind) {
        default: {
            value.handle = nullptr;
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto _type = type->As<Backend::IL::StructType>();
            
            value.values = std::span(
                arena.AllocateArray<IL::DebugSingleValue>(static_cast<uint32_t>(_type->memberTypes.size())),
                _type->memberTypes.size()
            );
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                CreateVariableValues(_type->memberTypes[i], arena, value.values[i] = {});
            }
            
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto _type = type->As<Backend::IL::ArrayType>();
            
            value.values = std::span(
                arena.AllocateArray<IL::DebugSingleValue>(_type->count),
                _type->count
            );
                
            for (uint32_t i = 0; i < _type->count; i++) {
                CreateVariableValues(_type->elementType, arena, value.values[i] = {});
            }

            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto _type = type->As<Backend::IL::VectorType>();
            
            value.values = std::span(
                arena.AllocateArray<IL::DebugSingleValue>(_type->dimension),
                _type->dimension
            );
                
            for (uint32_t i = 0; i < _type->dimension; i++) {
                CreateVariableValues(_type->containedType, arena, value.values[i] = {});
            }

            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto _type = type->As<Backend::IL::MatrixType>();
            
            value.values = std::span(
                arena.AllocateArray<IL::DebugSingleValue>(_type->columns * _type->rows),
                _type->columns * _type->rows
            );
                
            for (uint32_t column = 0; column < _type->columns; column++) {
                for (uint32_t row = 0; row < _type->rows; row++) {
                    CreateVariableValues(_type->containedType, arena, value.values[column * _type->rows + row] = {});
                }
            }

            break;
        }
    }
}

static const Backend::IL::Type* ConstructType(IL::Program& program, const SpvDebugMap& map, SpvId id) {
    const SpvDebugTypeInfo &type = map.typeInfos.at(id);
    switch (type.kind) {
        default: {
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType { });
        }
        case NonSemanticShaderDebugInfo100DebugTypeBasic: {
            switch (program.GetConstants().GetConstant<IL::IntConstant>(type.operands[2])->value) {
                default: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType { });
                }
                case NonSemanticShaderDebugInfo100Unspecified: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType { });
                }
                case NonSemanticShaderDebugInfo100Address: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType { 
                        .pointee = program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType { })
                    });
                }
                case NonSemanticShaderDebugInfo100Boolean: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType { });
                }
                case NonSemanticShaderDebugInfo100Float: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType { 
                        .bitWidth = static_cast<uint8_t>(program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])->value)
                    });
                }
                case NonSemanticShaderDebugInfo100Signed: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType { 
                        .bitWidth = static_cast<uint8_t>(program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])->value),
                        .signedness = true
                    });
                }
                case NonSemanticShaderDebugInfo100SignedChar: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType { 
                        .bitWidth = 8,
                        .signedness = true
                    });
                }
                case NonSemanticShaderDebugInfo100Unsigned: {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType { 
                        .bitWidth = static_cast<uint8_t>(program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])->value),
                        .signedness = false
                    });
                }
                case NonSemanticShaderDebugInfo100UnsignedChar : {
                    return program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType { 
                        .bitWidth = 8,
                        .signedness = false
                    });
                }
            }
            break;
        }
        case NonSemanticShaderDebugInfo100DebugTypePointer: {
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType { 
                .pointee = ConstructType(program, map, type.operands[0])
            });
        }
        case NonSemanticShaderDebugInfo100DebugTypeQualifier: {
            return ConstructType(program, map, type.operands[0]);
        }
        case NonSemanticShaderDebugInfo100DebugTypeArray: {
            uint32_t count = 1;
            
            if (const IL::IntConstant *constant = program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])) {
                count = static_cast<uint32_t>(constant->value);
            }
            
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::ArrayType { 
                .elementType = ConstructType(program, map, type.operands[0]),
                .count = static_cast<uint8_t>(count)
            });
        }
        case NonSemanticShaderDebugInfo100DebugTypeVector: {
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType { 
                .containedType = ConstructType(program, map, type.operands[0]),
                .dimension = static_cast<uint8_t>(program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])->value)
            });
        }
        case NonSemanticShaderDebugInfo100DebugTypeMatrix: {
            // TODO: Majorness
            
            auto* vectorType = ConstructType(program, map, type.operands[0])->As<Backend::IL::VectorType>();
            
            uint32_t count = 1;
            if (const IL::IntConstant *constant = program.GetConstants().GetConstant<IL::IntConstant>(type.operands[1])) {
                count = static_cast<uint32_t>(constant->value);
            }
            
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::MatrixType { 
                .containedType = vectorType->containedType,
                .rows = vectorType->dimension,
                .columns = static_cast<uint8_t>(count)
            });
            break;
        }
        case NonSemanticShaderDebugInfo100DebugTypedef: {
            return ConstructType(program, map, type.operands[0]);
        }
        case NonSemanticShaderDebugInfo100DebugTypeFunction: {
            Backend::IL::FunctionType fn;
            
            fn.returnType = ConstructType(program, map, type.operands[1]);
            
            for (uint32_t i = 2; i < type.opCount; i++) {
                fn.parameterTypes.push_back(ConstructType(program, map, type.operands[i]));
            }
            
            return program.GetTypeMap().FindTypeOrAdd(fn);
        }
        case NonSemanticShaderDebugInfo100DebugTypeEnum: {
            return ConstructType(program, map, type.operands[1]);
        }
        case NonSemanticShaderDebugInfo100DebugTypeComposite: {
            Backend::IL::StructType str;
            
            for (uint32_t i = 9; i < type.opCount; i++) {
                str.memberTypes.push_back(ConstructType(program, map, type.operands[i]));
            }
            
            return program.GetTypeMap().FindTypeOrAdd(str);
        }
        case NonSemanticShaderDebugInfo100DebugTypeMember: {
            return ConstructType(program, map, type.operands[1]);
        }
        case NonSemanticShaderDebugInfo100DebugTypeInheritance: {
            return ConstructType(program, map, type.operands[0]);
        }
        case NonSemanticShaderDebugInfo100DebugTypePtrToMember: {
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType { 
                .pointee = ConstructType(program, map, type.operands[0])
            });
        }
        case NonSemanticShaderDebugInfo100DebugTypeTemplate: {
            return ConstructType(program, map, type.operands[0]);
        }
        case NonSemanticShaderDebugInfo100DebugTypeTemplateParameter:
        case NonSemanticShaderDebugInfo100DebugTypeTemplateTemplateParameter:
        case NonSemanticShaderDebugInfo100DebugTypeTemplateParameterPack: {
            return program.GetTypeMap().FindTypeOrAdd(Backend::IL::UnexposedType { });
        }
    }
}

template<typename F>
void IterateDebugValues(IL::Program& program, const SpvDebugMap *debugMap, const IL::Instruction* instr, F&& functor) {
    // If store, find the delegating variable
    if (auto* storeInstr = instr->Cast<IL::StoreInstruction>()) {
        TrivialStackVector<uint32_t, 8u> indices;
        
        IL::ID address = storeInstr->address;
        
        // Keep walking until we hit a variable
        for (;;) {
            // Has a logical variable?
            if (auto it = debugMap->bindingInfos.find(address); it != debugMap->bindingInfos.end()) {
                const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(it->second.debugVariable);
                
                // Reverse in-place to get the forward order
                std::ranges::reverse(indices);
            
                InstructionValueInfo valueInfo;
                valueInfo.value = storeInstr->value;
                valueInfo.accessIndices = std::span(indices.Data(), indices.Size());
                functor(variableInfo, valueInfo);
                break;
            }

            // From another instruction?
            IL::InstructionRef addressInstrRef = program.GetIdentifierMap().Get(address);
            if (!instr) {
                break;
            }
            
            // Must be an address chain, otherwise not something we can reconstruct
            auto* addressInstr = addressInstrRef->Cast<IL::AddressChainInstruction>();
            if (!addressInstr) {
                break;
            }

            // Append in reverse
            for (int32_t i = static_cast<int32_t>(addressInstr->chains.count) - 1; i >= 0; i--) {
                indices.Add(addressInstr->chains[i].index);
            }
            
            // Onto the next base
            address = addressInstr->composite;
        }
    }
    
    // Has direct value info?
    auto it = debugMap->instructionValueInfos.find(instr->source.codeOffset);
    if (it != debugMap->instructionValueInfos.end()) {
        for (const InstructionValueInfo &valueInfo: it->second.values) {
            const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(valueInfo.debugVariableId);
            functor(variableInfo, valueInfo);
        }
    }
}

static IL::DebugSingleValue* GetStructuredValueAtOffsetRef(IL::Program& program, IL::DebugSingleValue& value, std::span<const uint32_t> indices) {
    IL::DebugSingleValue* valueInst = &value;
    
    for (uint32_t i = 0; i < indices.size(); i++) {
        switch (valueInst->type->kind) {
            default: {
                return nullptr;
            }
            case Backend::IL::TypeKind::Struct:
            case Backend::IL::TypeKind::Array:
            case Backend::IL::TypeKind::Vector:
            case Backend::IL::TypeKind::Matrix: {
                SpvId id = indices[i];

                // May be dynamic
                if (auto* constant = program.GetConstants().GetConstant<IL::IntConstant>(id)) {
                    valueInst = &valueInst->values[constant->value];
                } else {
                    // TODO: How do we handle dynamic offsets?
                    // We'd probably have to handle it at reconstruction, but that messes with this handling entirely
                    return nullptr;
                }
                break;
            }
        }
    }
    
    return valueInst;
}

static void PropagateValue(SmallArena& arena, IL::ID valueId, const Backend::IL::Type *valueType, IL::DebugSingleValue& value, TrivialStackVector<uint32_t, 4u>&  accessIndices) {
    switch (value.type->kind) {
        default: {
            // Since we're traversing in reverse, accept the latest value
            if (value.handle) {
                return;
            }
            
            uint32_t* dst = arena.AllocateArray<uint32_t>(static_cast<uint32_t>(accessIndices.Size()));
            std::memcpy(dst, accessIndices.Data(), accessIndices.Size() * sizeof(uint32_t));
            
            value.handle = arena.Allocate<DebugHandle>(DebugHandle {
                .result = valueId,
                .accessIndices = std::span(dst, accessIndices.Size())
            });
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto* _type = value.type->As<Backend::IL::ArrayType>();
            auto* _valueType = valueType->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                accessIndices.Add(i);
                PropagateValue(arena, valueId, _valueType->elementType, value.values[i], accessIndices);
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto* _type = value.type->As<Backend::IL::VectorType>();
            auto* _valueType = valueType->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                accessIndices.Add(i);
                PropagateValue(arena, valueId, _valueType->containedType, value.values[i], accessIndices);
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* _type = value.type->As<Backend::IL::StructType>();
            auto* _valueType = valueType->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                accessIndices.Add(i);
                PropagateValue(arena, valueId, _valueType->memberTypes[i], value.values[i], accessIndices);
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto* _type = value.type->As<Backend::IL::MatrixType>();
            auto* _valueType = valueType->As<Backend::IL::MatrixType>();
            
            for (uint32_t column = 0; column < _type->columns; column++) {
                accessIndices.Add(column);
                
                for (uint32_t row = 0; row < _type->rows; row++) {
                    accessIndices.Add(row);
                    PropagateValue(
                        arena, valueId,
                        _valueType->containedType, 
                        value.values[column * _type->rows + row],
                        accessIndices
                    );
                    accessIndices.PopBack();
                }
                
                accessIndices.PopBack();
            }
            break;
        }
    }
}

static IL::ID ReconstructStructuredValue(IL::Emitter<> &emitter, const IL::DebugSingleValue& value) {
    TrivialStackVector<IL::ID, 4u> valueStack;
    
    // Has handle?
    auto* handle = static_cast<DebugHandle*>(value.handle);
    
    switch (value.type->kind) {
        default: {
            // No handle? Default it
            if (!handle) {
                return emitter.GetProgram()->GetConstants().FindConstantOrAdd(value.type, IL::NullConstant {})->id;
            }
            
            IL::ID result = handle->result;
            
            // Extract until we get to the leaf
            for (uint32_t index : handle->accessIndices) {
                result = emitter.Extract(result, emitter.UInt32(index));
            } 
            
            return result;
        }
        case Backend::IL::TypeKind::Struct: {
            auto _type = value.type->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, value.values[i]));
            }
            
            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Array: {
            auto _type = value.type->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, value.values[i]));
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Vector: {
            auto _type = value.type->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, value.values[i]));
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Matrix: {
            auto _type = value.type->As<Backend::IL::MatrixType>();
            
            for (uint32_t column = 0; column < _type->columns; column++) {
                for (uint32_t row = 0; row < _type->rows; row++) {
                    valueStack.Add(ReconstructStructuredValue(emitter, value.values[column]));
                }
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
    }
}

void DebugEmitter::GetStack(IL::Program &program, const IL::Instruction *instr, SmallArena& arena, IL::DebugStack& stack) {
    // Get shader state
    ShaderModuleState* shaderState = table->states_shaderModule.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->spirvModule) {
        return;
    }

    // Must have debug map
    const SpvDebugMap *debugMap = shaderState->spirvModule->GetDebugMap();
    if (!debugMap) {
        return;
    }
    
    // Get traceback
    SpvCodeOffsetTraceback traceback = shaderState->spirvModule->GetCodeOffsetTraceback(instr->source.codeOffset);
    
    // Try to get the function
    IL::Function *fn = program.GetFunctionList().GetFunction(traceback.functionID);
    if (!fn) {
        return;
    }
    
    // Compute dominator analysis for walking
    ComRef<IL::DominatorAnalysis> dominatorAnalysis = fn->GetAnalysisMap().FindPassOrCompute<IL::DominatorAnalysis>(*fn);
    if (!dominatorAnalysis) {
        return;
    }

    IL::BasicBlockList &blocks = fn->GetBasicBlocks();
    
    std::unordered_map<uint32_t, IL::DebugVariable*> variables;
    
    // Starting basic block
    IL::BasicBlock* basicBlock = blocks.GetBlock(traceback.basicBlockID);
    
    // Instruction cache
    std::vector<const IL::Instruction*> instructions;
    
    // Keep walking
    while (basicBlock) {
        // Walk forward
        for (const IL::Instruction* blockInstr : *basicBlock) {
            instructions.push_back(blockInstr);
            
            if (blockInstr == instr) {
                break;
            }
        }
        
        // Walk back from IOI
        for (auto it = instructions.rbegin() ; it != instructions.rend(); ++it) {
            // Walk all values encoded
            IterateDebugValues(program, debugMap, *it, [&](const SpvDebugVariableInfo &varInfo, const InstructionValueInfo& valueInfo) {
                IL::DebugVariable* variable = nullptr;
                
                // Get or create
                if (auto varIt = variables.find(varInfo.varId); varIt != variables.end()) {
                    variable = varIt->second;
                } else {
                    // Get name of the variable
                    std::string_view name = debugMap->Get(varInfo.nameId, SpvOpString);
                    
                    // Construct the type
                    const Backend::IL::Type *varType = ConstructType(program, *debugMap, varInfo.typeId);
                    
                    // Create variable
                    variable = stack.variables.emplace_back(arena.Allocate<IL::DebugVariable>());
                    variable->name = name;
                    CreateVariableValues(varType, arena, variable->value);
                    
                    variables.insert(std::make_pair(varInfo.varId, variable));
                }
                
                // Traverse to the value to be reconstructed
                IL::DebugSingleValue* value = GetStructuredValueAtOffsetRef(program, variable->value, valueInfo.accessIndices);
                if (!value) {
                    return;
                }
                    
                // Type for composite checks
                const Backend::IL::Type *valueType = program.GetTypeMap().GetType(valueInfo.value);
                
                // Propagate to all leaf values
                TrivialStackVector<uint32_t, 4u> accessIndices;
                PropagateValue(arena, valueInfo.value, valueType, *value, accessIndices);
            });
        }
        
        instructions.clear();
        
        // Walk to the idom
        IL::BasicBlock *idom = dominatorAnalysis->GetImmediateDominator(basicBlock);
        if (idom == basicBlock) {
            break;
        }
        
        basicBlock = idom;
    }
}

IL::ID DebugEmitter::ReconstructValue(IL::Emitter<> &emitter, const IL::DebugSingleValue& value, const IL::Instruction *instr) {
    // Reconstruct the structured type
    return ReconstructStructuredValue(emitter, value);
}
