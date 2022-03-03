#pragma once

// Std
#include <list>
#include <map>

// Backend
#include "Variable.h"

namespace IL {
    struct VariableList {
        using Container = std::vector<Backend::IL::Variable>;

        VariableList(const Allocators& allocators, IdentifierMap& map) : allocators(allocators), map(map) {

        }

        /// Add a new variable
        void Add(const Backend::IL::Variable& arg) {
            variables.push_back(arg);
        }

        /// Remove a variable
        /// \param variable iterator
        void Remove(const Container::const_iterator& variable) {
            variables.erase(variable);
        }

        /// Copy this variable list
        /// \param out the new variable list
        void CopyTo(VariableList& out) const {
            out.revision = revision;
            out.variables = variables;
        }

        /// Iterator accessors
        Container::iterator begin() { return variables.begin(); }
        Container::reverse_iterator rbegin() { return variables.rbegin(); }
        Container::iterator end() { return variables.end(); }
        Container::reverse_iterator rend() { return variables.rend(); }
        Container::const_iterator begin() const { return variables.begin(); }
        Container::const_reverse_iterator rbegin() const { return variables.rbegin(); }
        Container::const_iterator end() const { return variables.end(); }
        Container::const_reverse_iterator rend() const { return variables.rend(); }

    private:
        Allocators allocators;

        /// The shared identifier map
        IdentifierMap& map;

        /// All variables
        Container variables;

        /// Basic block revision
        uint32_t revision{0};
    };
}
