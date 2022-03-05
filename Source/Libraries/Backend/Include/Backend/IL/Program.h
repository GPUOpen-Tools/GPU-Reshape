#pragma once

// Backend
#include "Function.h"
#include "IdentifierMap.h"
#include "TypeMap.h"
#include "FunctionList.h"

// Std
#include <list>
#include <algorithm>

namespace IL {
    struct Program {
        Program(const Allocators &allocators, uint64_t shaderGUID) :
            allocators(allocators),
            typeMap(allocators),
            functions(allocators, identifierMap),
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
            program->typeMap = typeMap.Copy();

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

        /// Get the identifier map
        Backend::IL::TypeMap &GetTypeMap() {
            return typeMap;
        }

        /// Get the identifier map
        FunctionList &GetFunctionList() {
            return functions;
        }

        /// Get the identifier map
        const FunctionList &GetFunctionList() const {
            return functions;
        }

        /// Get the identifier map
        const Backend::IL::TypeMap &GetTypeMap() const {
            return typeMap;
        }

        /// Get the identifier map
        const IdentifierMap &GetIdentifierMap() const {
            return identifierMap;
        }

    private:
        Allocators allocators;

        /// Functions within this program
        FunctionList functions;

        /// The identifier map
        IdentifierMap identifierMap;

        /// The type map
        Backend::IL::TypeMap typeMap;

        /// Shader guid of this program
        uint64_t shaderGUID{~0ull};
    };
}
