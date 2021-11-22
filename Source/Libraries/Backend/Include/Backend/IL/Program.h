#pragma once

// Backend
#include "Function.h"
#include "IdentifierMap.h"

// Std
#include <list>
#include <algorithm>

namespace IL {
    struct Program {
        Program(const Allocators& allocators) : allocators(allocators) {

        }

        // No move
        Program(Program&& other) = delete;
        Program& operator=(Program&& other) = delete;

        // No copy
        Program(const Program& other) = delete;
        Program& operator=(const Program& other) = delete;

        /// Copy this program
        /// \return
        Program* Copy() const {
            auto program = new (allocators) Program(allocators);
            program->map.SetBound(map.GetMaxID());

            // Copy all functions
            for (const Function& fn : functions) {
                program->functions.emplace_back(fn.Copy(program->map));
            }

            return program;
        }

        /// Allocate a new basic block
        /// \return
        Function* AllocFunction(ID id) {
            functions.emplace_back(allocators, map, id);
            return &functions.back();
        }

        /// Get the number of functions
        uint32_t GetFunctionCount() const {
            return static_cast<uint32_t>(functions.size());
        }

        /// Iterator
        std::list<Function>::iterator begin() {
            return functions.begin();
        }

        /// Iterator
        std::list<Function>::iterator end() {
            return functions.end();
        }

        /// Iterator
        std::list<Function>::const_iterator begin() const {
            return functions.begin();
        }

        /// Iterator
        std::list<Function>::const_iterator end() const {
            return functions.end();
        }

        /// Get the identifier map
        IdentifierMap& GetIdentifierMap() {
            return map;
        }

        /// Get the identifier map
        const IdentifierMap& GetIdentifierMap() const {
            return map;
        }

    private:
        Allocators allocators;

        /// Functions within this program
        std::list<Function> functions;

        /// The identifier map
        IdentifierMap map;
    };
}
