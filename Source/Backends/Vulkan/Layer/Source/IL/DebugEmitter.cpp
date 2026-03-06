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

// Spirv
#include <spirv/unified1/NonSemanticShaderDebugInfo100.h>

struct DebugHandle {
    /// Assigned value
    IL::ID result;
};

DebugEmitter::DebugEmitter(DeviceDispatchTable* table) : table(table) {
    
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

    // If store, find the delegating variable
    if (auto* storeInstr = instr->Cast<IL::StoreInstruction>()) {
        if (auto it = debugMap->bindingInfos.find(storeInstr->address); it != debugMap->bindingInfos.end()) {
            const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(it->second.debugVariable);
            
            // Create variable
            IL::DebugVariable *dest = stack.variables.emplace_back(arena.Allocate<IL::DebugVariable>());
            dest->name = debugMap->Get(variableInfo.nameId, SpvOpString);
            dest->value.type = ConstructType(program, *debugMap, variableInfo.typeId);
            dest->value.handle = arena.Allocate<DebugHandle>(DebugHandle {
                .result = storeInstr->value
            });
        }
    }
    
    // Has direct value info?
    auto it = debugMap->instructionValueInfos.find(instr->source.codeOffset);
    if (it != debugMap->instructionValueInfos.end()) {
        for (const InstructionValueInfo &valueInfo: it->second.values) {
            const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(valueInfo.debugVariableId);

            // Create variable
            IL::DebugVariable *dest = stack.variables.emplace_back(arena.Allocate<IL::DebugVariable>());
            dest->name = debugMap->Get(variableInfo.nameId, SpvOpString);
            dest->value.type = ConstructType(program, *debugMap, variableInfo.typeId);
            dest->value.handle = arena.Allocate<DebugHandle>(DebugHandle {
                .result = valueInfo.value
            });
        }
    }
}

IL::ID DebugEmitter::ReconstructValue(IL::Emitter<> &emitter, const IL::DebugSingleValue& value, const IL::Instruction *instr) {
    // Nothing to reconstruct
    return static_cast<DebugHandle*>(value.handle)->result;
}
