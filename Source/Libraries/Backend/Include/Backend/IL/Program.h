// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include "Function.h"
#include "IdentifierMap.h"
#include "TypeMap.h"
#include "ConstantMap.h"
#include "FunctionList.h"
#include "ShaderDataMap.h"
#include "CapabilityTable.h"

// Std
#include <list>
#include <algorithm>

namespace IL {
    struct Program {
        Program(const Allocators &allocators, uint64_t shaderGUID) :
            allocators(allocators),
            functions(allocators, identifierMap),
            variables(allocators, identifierMap),
            constants(allocators, identifierMap, typeMap, capabilityTable),
            typeMap(allocators, identifierMap, capabilityTable),
            shaderDataMap(identifierMap, typeMap),
            shaderGUID(shaderGUID) {
            /* */
        }

        // No move
        Program(Program &&other) = delete;
        Program &operator=(Program &&other) = delete;

        // No copy
        Program(const Program &other) = delete;
        Program &operator=(const Program &other) = delete;

        /// Copy this program
        /// \return
        Program *Copy() const {
            auto program = new(allocators) Program(allocators, shaderGUID);
            program->identifierMap.SetBound(identifierMap.GetMaxID());
            typeMap.CopyTo(program->typeMap);
            constants.CopyTo(program->constants);

            // Copy immutable
            program->capabilityTable = capabilityTable;
            program->entryPoint = entryPoint;

            // Copy all functions and their basic blocks
            functions.CopyTo(program->functions);

            // Reindex all users
            for (IL::Function* fn : program->functions) {
                fn->IndexUsers();
            }

            // OK
            return program;
        }

        /// Set the new entry point
        /// \param id must be valid function within the program
        void SetEntryPoint(IL::ID id) {
            entryPoint = id;
        }

        /// Get the entry point
        IL::Function* GetEntryPoint() const {
            return functions.GetFunction(entryPoint);
        }

        /// Get the shader guid
        uint64_t GetShaderGUID() const {
            return shaderGUID;
        }

        /// Get the identifier map
        IdentifierMap &GetIdentifierMap() {
            return identifierMap;
        }

        /// Get the type map
        Backend::IL::TypeMap &GetTypeMap() {
            return typeMap;
        }

        /// Get the shader Data map
        ShaderDataMap &GetShaderDataMap() {
            return shaderDataMap;
        }

        /// Get the identifier map
        FunctionList &GetFunctionList() {
            return functions;
        }

        /// Get the global variables map
        VariableList &GetVariableList() {
            return variables;
        }

        /// Get the capability table
        CapabilityTable& GetCapabilityTable() {
            return capabilityTable;
        }

        /// Get the global constants
        Backend::IL::ConstantMap &GetConstants() {
            return constants;
        }

        /// Get the identifier map
        const FunctionList &GetFunctionList() const {
            return functions;
        }

        /// Get the global variables map
        const VariableList &GetVariableList() const {
            return variables;
        }

        /// Get the identifier map
        const Backend::IL::TypeMap &GetTypeMap() const {
            return typeMap;
        }

        /// Get the shader Data map
        const ShaderDataMap &GetShaderDataMap() const {
            return shaderDataMap;
        }

        /// Get the identifier map
        const IdentifierMap &GetIdentifierMap() const {
            return identifierMap;
        }

        /// Get the global constants
        const Backend::IL::ConstantMap &GetConstants() const {
            return constants;
        }

        /// Get the capability table
        const CapabilityTable& GetCapabilityTable() const {
            return capabilityTable;
        }

    private:
        Allocators allocators;

        /// Functions within this program
        FunctionList functions;

        /// Global variables
        VariableList variables;

        /// Global constants
        Backend::IL::ConstantMap constants;

        /// The identifier map
        IdentifierMap identifierMap;

        /// The type map
        Backend::IL::TypeMap typeMap;

        /// User generated shader data
        ShaderDataMap shaderDataMap;

        /// The capability table
        CapabilityTable capabilityTable;

        /// Function entry point
        IL::ID entryPoint{IL::InvalidID};

        /// Shader guid of this program
        uint64_t shaderGUID{~0ull};
    };
}
