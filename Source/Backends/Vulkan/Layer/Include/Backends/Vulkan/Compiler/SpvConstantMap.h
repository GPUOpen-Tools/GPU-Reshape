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

// Layer
#include <Backends/Vulkan/Compiler/SpvTypeMap.h>

// Backend
#include <Backend/IL/ConstantMap.h>
#include <Backend/IL/IdentifierMap.h>
#include <Backend/IL/Constant.h>
#include <Backend/IL/Type.h>

// SPIRV
#include <spirv/unified1/spirv.h>

// Std
#include <vector>
#include <bit>

class SpvConstantMap {
public:
    SpvConstantMap(Backend::IL::ConstantMap &programMap, SpvTypeMap &typeMap) :
        programMap(programMap),
        typeMap(typeMap) {

    }

    /// Copy this constant map
    /// \param out destination map
    void CopyTo(SpvConstantMap &out) {
        out.constants = constants;
        out.constantLookup = constantLookup;
    }

    /// Set the declaration stream for allocations
    void SetDeclarationStream(SpvStream *value) {
        declarationStream = value;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant *AddConstant(IL::ID id, const typename T::Type *type, const T &constant) {
        const auto *constantPtr = programMap.AddConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant *AddUnsortedConstant(IL::ID id, const Backend::IL::Type *type, const T &constant) {
        const auto *constantPtr = programMap.AddUnsortedConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Get constant at offset
    /// \param id source id
    /// \return nullptr if not found
    const IL::Constant *GetConstant(uint32_t id) {
        if (id >= constants.size()) {
            return nullptr;
        }

        return constants[id];
    }

    /// Get a constant from a type
    /// \param type IL type
    void EnsureConstant(const Backend::IL::Constant *constant) {
        // Allocate if need be
        if (!HasConstant(constant)) {
            CompileConstant(constant);
        }
    }

    /// Add a new constant mapping
    /// \param type IL type
    /// \param index SPIRV id
    void AddConstantMapping(const Backend::IL::Constant *constant, uint64_t index) {
        if (constantLookup.size() <= constant->id) {
            constantLookup.resize(constant->id + 1, ~0u);
        }

        constantLookup[constant->id] = index;
    }

    /// Check if a constant is present in SPIRV
    /// \param type IL type
    /// \return true if present
    bool HasConstant(const Backend::IL::Constant *type) {
        return type->id < constantLookup.size() && constantLookup[type->id] != ~0u;
    }

private:
    /// Compile a given constant
    /// \param constant
    /// \return SPIRV id
    void CompileConstant(const Backend::IL::Constant *constant) {
        SpvId typeId = typeMap.GetSpvTypeId(constant->type);
        
        switch (constant->kind) {
            default:
                ASSERT(false, "Unsupported constant type for recompilation");
                break;
            case Backend::IL::ConstantKind::Bool:
                CompileConstant(constant->As<IL::BoolConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::Int:
                CompileConstant(constant->As<IL::IntConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::FP:
                CompileConstant(constant->As<IL::FPConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::Null:
                CompileConstant(constant->As<IL::NullConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::Struct:
                CompileConstant(constant->As<IL::StructConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::Vector:
                CompileConstant(constant->As<IL::VectorConstant>(), typeId);
                break;
            case Backend::IL::ConstantKind::Array:
                CompileConstant(constant->As<IL::ArrayConstant>(), typeId);
                break;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::BoolConstant *constant, SpvId typeId) {
        if (constant->value) {
            SpvInstruction &spv = declarationStream->Allocate(SpvOpConstantTrue, 3);
            spv[1] = typeId;
            spv[2] = constant->id;
        } else {
            SpvInstruction &spv = declarationStream->Allocate(SpvOpConstantFalse, 3);
            spv[1] = typeId;
            spv[2] = constant->id;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::IntConstant *constant, SpvId typeId) {
        auto* intType = constant->type->As<Backend::IL::IntType>();

        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstant, 3 + (intType->bitWidth + 31) / 32);
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;

        for (uint32_t i = 0; i < intType->bitWidth; i += 32) {
            spvOffset[3 + i] |= std::bit_cast<uint64_t>(constant->value) >> i;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::FPConstant *constant, SpvId typeId) {
        auto* fpType = constant->type->As<Backend::IL::FPType>();

        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstant, 3 + (fpType->bitWidth + 31) / 32);
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;

        for (uint32_t i = 0; i < fpType->bitWidth; i += 32) {
            spvOffset[3 + i] |= std::bit_cast<uint64_t>(constant->value) >> i;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::NullConstant *constant, SpvId typeId) {
        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstantNull, 3);
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::StructConstant *constant, SpvId typeId) {
        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstantComposite, 3 + static_cast<uint32_t>(constant->members.size()));
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;

        for (size_t i = 0; i < constant->members.size(); i++) {
            spvOffset[3 + static_cast<uint32_t>(i)] = constant->members[i]->id;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::VectorConstant *constant, SpvId typeId) {
        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstantComposite, 3 + static_cast<uint32_t>(constant->elements.size()));
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;

        for (size_t i = 0; i < constant->elements.size(); i++) {
            spvOffset[3 + static_cast<uint32_t>(i)] = constant->elements[i]->id;
        }
    }

    /// Compile a given constant
    void CompileConstant(const Backend::IL::ArrayConstant *constant, SpvId typeId) {
        SpvInstruction &spvOffset = declarationStream->Allocate(SpvOpConstantComposite, 3 + static_cast<uint32_t>(constant->elements.size()));
        spvOffset[1] = typeId;
        spvOffset[2] = constant->id;

        for (size_t i = 0; i < constant->elements.size(); i++) {
            spvOffset[3 + static_cast<uint32_t>(i)] = constant->elements[i]->id;
        }
    }

private:
    /// IL map
    Backend::IL::ConstantMap &programMap;

    /// All constants
    std::vector<const IL::Constant *> constants;

    /// IL type to SPIRV type table
    std::vector<uint64_t> constantLookup;

    /// Shared type map
    SpvTypeMap &typeMap;

    /// External declaration stream for allocations
    SpvStream *declarationStream{nullptr};
};
