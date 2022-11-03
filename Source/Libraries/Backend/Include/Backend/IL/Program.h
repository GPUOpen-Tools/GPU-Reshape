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
            typeMap(allocators, identifierMap),
            constants(allocators, identifierMap, typeMap),
            functions(allocators, identifierMap),
            variables(allocators, identifierMap),
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

            // Copy capabilities
            program->capabilityTable = capabilityTable;

            // Copy all functions and their basic blocks
            functions.CopyTo(program->functions);

            // Reindex all users
            for (IL::Function* fn : program->functions) {
                fn->IndexUsers();
            }

            // OK
            return program;
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

        /// Shader guid of this program
        uint64_t shaderGUID{~0ull};
    };
}
