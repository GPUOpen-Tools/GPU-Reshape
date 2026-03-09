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

#include <Backends/DX12/IL/DebugEmitter.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/IDXDebugModule.h>
#include <Backends/DX12/Compiler/IDXModule.h>

// Backend
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>
#include <Backend/IL/Tiny/TinyType.h>
#include <Backend/IL/TypeSize.h>

struct DebugHandle {
    /// Assigned value code
    DXDwarfCode value;
    
    /// Optional, access indices for code values
    std::span<uint32_t> accessIndices;
};

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

    
static uint32_t GetComponentCountRecursive(const IL::Constant* constant) {
    switch (constant->kind) {
        default:
            return 1u;
        case Backend::IL::ConstantKind::Vector:
            return static_cast<uint32_t>(constant->As<IL::VectorConstant>()->elements.size());
        case Backend::IL::ConstantKind::Array:
            return static_cast<uint32_t>(constant->As<IL::ArrayConstant>()->elements.size());
        case Backend::IL::ConstantKind::Struct: {
            uint32_t count = 0;
            for (const IL::Constant * memberType : constant->As<IL::StructConstant>()->members) {
                count += GetComponentCountRecursive(memberType);
            }
            return count;
        }
    }
}

static IL::DebugSingleValue* GetStructuredValueAtOffsetRef(IL::DebugSingleValue& value, const IL::Constant* constant, uint64_t& byteOffset) {
    // Do a component wise match if we're at base
    if (!byteOffset && constant) {
        // Just assume we're at base if the components match
        // Really, we should reconstruct the type table properly of the debug module, but until then
        if (Backend::IL::GetComponentCountRecursive(value.type) && GetComponentCountRecursive(constant)) {
            return &value;
        }
    }
    
    switch (value.type->kind) {
        default: {
            const uint64_t byteSize = Backend::IL::GetPODNonAlignedTypeByteSize(value.type);
            if (byteOffset < byteSize) {
                return &value;
            }

            byteOffset -= byteSize;
            return nullptr;
        }
        case Backend::IL::TypeKind::Struct:
        case Backend::IL::TypeKind::Array:
        case Backend::IL::TypeKind::Vector:
        case Backend::IL::TypeKind::Matrix: {
            for (IL::DebugSingleValue& member : value.values) {
                if (IL::DebugSingleValue* result = GetStructuredValueAtOffsetRef(member, constant, byteOffset)) {
                    return result;
                }
            }

            return nullptr;
        }
    }
}

static const IL::Constant* ReconstructConstant(IL::Emitter<> &emitter, const Backend::IL::Type* type, const IL::Constant* constant) {
    switch (constant->kind) {
        default: {
            ASSERT(false, "Invalid type");
            return nullptr;
        }
        case Backend::IL::ConstantKind::Unexposed: {
            return constant;
        }
        case Backend::IL::ConstantKind::Null: {
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(type, IL::NullConstant {});
        }
        case Backend::IL::ConstantKind::Bool: {
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(type->As<Backend::IL::BoolType>(), IL::BoolConstant {
                .value = constant->As<IL::BoolConstant>()->value
            });
        }
        case Backend::IL::ConstantKind::Int: {
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(type->As<Backend::IL::IntType>(), IL::IntConstant {
                .value = constant->As<IL::IntConstant>()->value
            });
        }
        case Backend::IL::ConstantKind::FP: {
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(type->As<Backend::IL::FPType>(), IL::FPConstant {
                .value = constant->As<IL::FPConstant>()->value
            });
        }
        case Backend::IL::ConstantKind::Array: {
            IL::ArrayConstant decl;
            
            auto* _type = type->As<Backend::IL::ArrayType>();
            
            for (const IL::Constant* element : constant->As<IL::ArrayConstant>()->elements) {
                decl.elements.push_back(ReconstructConstant(emitter, _type->elementType, element));
            } 
            
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(_type, decl);
        }
        case Backend::IL::ConstantKind::Vector: {
            IL::VectorConstant decl;
            
            auto* _type = type->As<Backend::IL::VectorType>();
            
            for (const IL::Constant* element : constant->As<IL::VectorConstant>()->elements) {
                decl.elements.push_back(ReconstructConstant(emitter, _type->containedType, element));
            } 
            
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(_type, decl);
        }
        case Backend::IL::ConstantKind::Struct: {
            IL::StructConstant decl;
            
            auto* _type = type->As<Backend::IL::StructType>();
            auto* _struct = constant->As<IL::StructConstant>();
            
            for (uint32_t i = 0; i < _struct->members.size(); i++) {
                decl.members.push_back(ReconstructConstant(emitter, _type->memberTypes[i], _struct->members[i]));
            }
            
            return emitter.GetProgram()->GetConstants().FindConstantOrAdd(_type, decl);
        }
    }
}

static void PropagateCodeConstant(SmallArena& arena, const IL::Constant* constant, IL::DebugSingleValue& value) {
    switch (value.type->kind) {
        default: {
            ASSERT(GetComponentCountRecursive(constant) == 1, "Structural constant in leaf");
            
            // Since we're traversing in reverse, accept the latest value
            if (value.handle) {
                return;
            }
            
            value.handle = arena.Allocate<DebugHandle>(DebugHandle {
                .value = {
                    .constant = constant
                }
            });
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto* _type = value.type->As<Backend::IL::ArrayType>();
            
            auto* _constant = constant->Cast<IL::ArrayConstant>();
            ASSERT(_constant || constant->Is<IL::NullConstant>(), "Unexpected constant");
            
            for (uint32_t i = 0; i < _type->count; i++) {
                PropagateCodeConstant(arena, _constant ? _constant->elements[i] : constant, value.values[i]);
            }
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto* _type = value.type->As<Backend::IL::VectorType>();
            
            auto* _constant = constant->Cast<IL::VectorConstant>();
            ASSERT(_constant || constant->Is<IL::NullConstant>(), "Unexpected constant");
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                PropagateCodeConstant(arena, _constant ? _constant->elements[i] : constant, value.values[i]);
            }
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* _type = value.type->As<Backend::IL::StructType>();
            
            auto* _constant = constant->Cast<IL::StructConstant>();
            ASSERT(_constant || constant->Is<IL::NullConstant>(), "Unexpected constant");
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                PropagateCodeConstant(arena, _constant ? _constant->members[i] : constant, value.values[i]);
            }
            break;
        }
        case Backend::IL::TypeKind::Matrix: {
            auto* _type = value.type->As<Backend::IL::MatrixType>();
            
            // DXIL allows representing it through vector constants
            if (constant->Is<IL::VectorConstant>()) {
                auto* _constant = constant->As<IL::VectorConstant>();
            
                for (uint32_t column = 0; column < _type->columns; column++) {
                    for (uint32_t row = 0; row < _type->rows; row++) {
                        PropagateCodeConstant(
                            arena, 
                            _constant->elements[column * _type->rows + row],
                            value.values[column * _type->rows + row]
                        );
                    }
                }
            } else {
                auto* _constant = constant->Cast<IL::ArrayConstant>();
                ASSERT(_constant || constant->Is<IL::NullConstant>(), "Unexpected constant");
            
                for (uint32_t column = 0; column < _type->columns; column++) {
                    for (uint32_t row = 0; row < _type->rows; row++) {
                        PropagateCodeConstant(
                            arena, 
                            _constant ? _constant->elements[column]->As<IL::VectorConstant>()->elements[row] : constant,
                            value.values[column * _type->rows + row]
                        );
                    }
                }
            }
            break;
        }
    }
}

static void PropagateCodeOffset(SmallArena& arena, uint32_t codeOffset, const Backend::IL::Type *valueType, IL::DebugSingleValue& value, TrivialStackVector<uint32_t, 4u>&  accessIndices) {
    switch (value.type->kind) {
        default: {
            // Since we're traversing in reverse, accept the latest value
            if (value.handle) {
                return;
            }
            
            // We're at a value leaf, but DXIL may occasionally host composite types on leafs where we're implicitly referencing
            // the first component. This is a bug, but we have to live with it.
            bool bIsImplicitLeaf = valueType->Is<Backend::IL::StructType>();
            if (bIsImplicitLeaf) {
                accessIndices.Add(0);
            }
            
            uint32_t* dst = arena.AllocateArray<uint32_t>(static_cast<uint32_t>(accessIndices.Size()));
            std::memcpy(dst, accessIndices.Data(), accessIndices.Size() * sizeof(uint32_t));
            
            value.handle = arena.Allocate<DebugHandle>(DebugHandle {
                .value = {
                    .codeOffset = codeOffset
                },
                .accessIndices = std::span(dst, accessIndices.Size())
            });
            
            // Cleanup
            if (bIsImplicitLeaf) {
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Array: {
            auto* _type = value.type->As<Backend::IL::ArrayType>();
            auto* _valueType = valueType->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                accessIndices.Add(i);
                PropagateCodeOffset(arena, codeOffset, _valueType->elementType, value.values[i], accessIndices);
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto* _type = value.type->As<Backend::IL::VectorType>();
            auto* _valueType = valueType->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                accessIndices.Add(i);
                PropagateCodeOffset(arena, codeOffset, _valueType->containedType, value.values[i], accessIndices);
                accessIndices.PopBack();
            }
            break;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* _type = value.type->As<Backend::IL::StructType>();
            auto* _valueType = valueType->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                accessIndices.Add(i);
                PropagateCodeOffset(arena, codeOffset, _valueType->memberTypes[i], value.values[i], accessIndices);
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
                    PropagateCodeOffset(
                        arena, codeOffset, 
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

static IL::ID ReconstructStructuredValue(IL::Emitter<> &emitter, IDXModule* module, const IL::DebugSingleValue& value) {
    TrivialStackVector<IL::ID, 4u> valueStack;
    
    // Has handle?
    auto* handle = static_cast<DebugHandle*>(value.handle);
    if (handle && handle->value.constant) {
        // Reconstruct the constant
        if (const IL::Constant *constant = ReconstructConstant(emitter, value.type, handle->value.constant)) {
            return constant->id;
        } else {
            return IL::InvalidID;
        }
    }
    
    switch (value.type->kind) {
        default: {
            // No handle? Default it
            if (!handle) {
                return emitter.GetProgram()->GetConstants().FindConstantOrAdd(value.type, IL::NullConstant {})->id;
            }
            
            // Get the traceback from the value (debug module -> canonical)
            DXCodeOffsetTraceback valueTraceback = module->GetCodeOffsetTraceback(handle->value.codeOffset);
            if (valueTraceback.instructionID == IL::InvalidID) {
                ASSERT(false, "Unexpected traceback state");
                return emitter.GetProgram()->GetConstants().FindConstantOrAdd(value.type, IL::NullConstant {})->id;
            }

            // Get instruction
            IL::InstructionRef valueInstr = emitter.GetProgram()->GetIdentifierMap().Get(valueTraceback.instructionID);
            if (valueInstr->result == IL::InvalidID) {
                ASSERT(false, "Unexpected instruction state");
                return emitter.GetProgram()->GetConstants().FindConstantOrAdd(value.type, IL::NullConstant {})->id;
            }
            
            IL::ID result = valueInstr;
            
            // Extract until we get to the leaf
            for (uint32_t index : handle->accessIndices) {
                result = emitter.Extract(result, emitter.UInt32(index));
            } 
            
            return result;
        }
        case Backend::IL::TypeKind::Struct: {
            auto _type = value.type->As<Backend::IL::StructType>();
            
            for (uint32_t i = 0; i < _type->memberTypes.size(); i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, module, value.values[i]));
            }
            
            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Array: {
            auto _type = value.type->As<Backend::IL::ArrayType>();
            
            for (uint32_t i = 0; i < _type->count; i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, module, value.values[i]));
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Vector: {
            auto _type = value.type->As<Backend::IL::VectorType>();
            
            for (uint32_t i = 0; i < _type->dimension; i++) {
                valueStack.Add(ReconstructStructuredValue(emitter, module, value.values[i]));
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
        case Backend::IL::TypeKind::Matrix: {
            auto _type = value.type->As<Backend::IL::MatrixType>();
            
            for (uint32_t column = 0; column < _type->columns; column++) {
                for (uint32_t row = 0; row < _type->rows; row++) {
                    valueStack.Add(ReconstructStructuredValue(emitter, module, value.values[column]));
                }
            }

            return emitter.ConstructPtr(value.type, valueStack.Data(), static_cast<uint32_t>(valueStack.Size()));
        }
    }
}

DebugEmitter::DebugEmitter(DeviceState *device) : device(device) {
    
}

void DebugEmitter::GetStack(IL::Program &program, const IL::Instruction *instr, SmallArena& arena, IL::DebugStack& stack) {
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return;
    }

    // Get traceback
    DXCodeOffsetTraceback traceback = shaderState->module->GetCodeOffsetTraceback(instr->source.codeOffset);
    
    // Try to get the function
    IL::Function *fn = program.GetFunctionList().GetFunction(traceback.functionID);
    if (!fn) {
        return;
    }

    // May not have debug module
    IDXDebugModule *module = shaderState->module->GetDebug();
    if (!module) {
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
            // Get the dwarf info for the code offset
            DXDwarfInfo info = module->GetDwarfInfo(program.GetTypeMap(), fn, (*it)->source.codeOffset);
    
            // Copy over variables
            for (uint64_t i = 0; i < info.variables.size(); ++i) {
                const DXDwarfVariableValue& source = info.variables[i];
                
                // Target type
                const Backend::IL::Type *type = source.type;
                if (!type) {
                    continue;
                }
                
                IL::DebugVariable* variable = nullptr;
                
                if (auto varIt = variables.find(source.variableId); varIt != variables.end()) {
                    variable = varIt->second;
                } else {
                    variable = stack.variables.emplace_back(arena.Allocate<IL::DebugVariable>());
                    variable->name = source.name;
                    CreateVariableValues(source.type, arena, variable->value);
                    
                    variables.insert(std::make_pair(source.variableId, variable));
                }
                
                for (const DXDwarfValue & dwarfValue : source.values) {
                    // TODO: Bit extraction?
                    ASSERT(dwarfValue.bitWise.bitStart % 8 == 0, "Non-byte aligned");
                    
                    uint64_t byteOffset = dwarfValue.bitWise.bitStart / 8;
                    IL::DebugSingleValue* value = GetStructuredValueAtOffsetRef(variable->value, dwarfValue.code.constant, byteOffset);
                
                    if (!value) {
                        ASSERT(false, "Failed to get structured value at offset");
                        continue;
                    }
                    
                    // To preserve the history, propagate the constant across all atomic units
                    if (dwarfValue.code.codeOffset != IL::InvalidID) {
                        DXCodeOffsetTraceback valueTraceback = shaderState->module->GetCodeOffsetTraceback(dwarfValue.code.codeOffset);
                        ASSERT(valueTraceback.instructionID != IL::InvalidID, "Unexpected traceback state");
                        
                        // Get instruction
                        IL::InstructionRef valueInstr = program.GetIdentifierMap().Get(valueTraceback.instructionID);
                        ASSERT(valueInstr->result != IL::InvalidID, "Unexpected instruction state");

                        // Type for composite checks
                        const Backend::IL::Type *valueType = program.GetTypeMap().GetType(valueInstr->result);
                        
                        TrivialStackVector<uint32_t, 4u> accessIndices;
                        PropagateCodeOffset(arena, dwarfValue.code.codeOffset, valueType, *value, accessIndices);
                    } else {
                        PropagateCodeConstant(arena, dwarfValue.code.constant, *value);
                    }
                }
            }
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
    IL::Program &program = *emitter.GetProgram();
    
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return IL::InvalidID;
    }
    
    // Reconstruct the structured type
    return ReconstructStructuredValue(emitter, shaderState->module, value);
}
