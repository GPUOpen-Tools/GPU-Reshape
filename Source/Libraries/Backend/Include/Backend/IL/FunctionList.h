#pragma once

#include <list>
#include <unordered_map>

#include "Function.h"

namespace IL {
    struct FunctionList {
        using Container = std::vector<Function*>;

        FunctionList(const Allocators& allocators, IdentifierMap& map) : allocators(allocators), map(map) {

        }

        /// Allocate a new function
        /// \return allocated function
        Function* AllocFunction(ID bid) {
            revision++;

            auto* function = new (allocators) Function(allocators, map, bid);
            functions.push_back(function);
            functionMap[bid] = function;
            return function;
        }

        /// Allocate a new basic block
        /// \return allocated basic block
        Function* AllocFunction() {
            return AllocFunction(map.AllocID());
        }

        /// Get a block from an identifier
        /// \param bid basic block identifier
        /// \return nullptr if not found
        Function* GetFunction(ID bid) const {
            auto it = functionMap.find(bid);
            if (it == functionMap.end()) {
                return nullptr;
            }

            return it->second;
        }

        /// Get the current basic block revision
        uint32_t GetRevision() const {
            return revision;
        }

        /// Remove a function
        /// \param function function iterator
        void Remove(const Container::const_iterator& function) {
            functionMap[(*function)->GetID()] = nullptr;
            functions.erase(function);
        }

        /// Add a function
        /// \param function function to be added
        void Add(Function* function) {
            functions.push_back(function);
            functionMap[function->GetID()] = function;
        }

        /// Swap functions with a container
        /// \param list the container to be swapped with
        void SwapFunctions(Container& list) {
            functions.swap(list);
        }

        /// Get the number of blocks
        uint32_t GetCount() const {
            return static_cast<uint32_t>(functions.size());
        }

        /// Copy this function
        /// \param copyMap the new identifier map
        void CopyTo(FunctionList& out) const {
            out.revision = revision;

            // Copy all basic blocks
            for (const Function* fn : functions) {
                auto* copy = new (allocators) Function(allocators, out.map, fn->GetID());
                fn->CopyTo(copy);

                out.functions.push_back(copy);
                out.functionMap[copy->GetID()] = copy;
            }
        }

        /// Iterator accessors
        Container::iterator begin() { return functions.begin(); }
        Container::reverse_iterator rbegin() { return functions.rbegin(); }
        Container::iterator end() { return functions.end(); }
        Container::reverse_iterator rend() { return functions.rend(); }
        Container::const_iterator begin() const { return functions.begin(); }
        Container::const_reverse_iterator rbegin() const { return functions.rbegin(); }
        Container::const_iterator end() const { return functions.end(); }
        Container::const_reverse_iterator rend() const { return functions.rend(); }

    private:
        Allocators allocators;

        /// The shared identifier map
        IdentifierMap& map;

        /// All blocks
        Container functions;

        /// Block map
        std::unordered_map<ID, Function*> functionMap;

        /// Basic block revision
        uint32_t revision{0};
    };
}
