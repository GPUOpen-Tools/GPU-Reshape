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

// Std
#include <list>
#include <map>

// Backend
#include "Variable.h"

namespace IL {
    struct VariableList {
        using Container = std::vector<const IL::Variable*>;

        VariableList(const Allocators& allocators, IdentifierMap& map) : allocators(allocators), map(map) {

        }

        /// Add a new variable
        void Add(const IL::Variable* arg) {
            variables.push_back(arg);
            variableMap[arg->id] = arg;
        }

        /// Remove a variable
        /// \param variable iterator
        void Remove(const Container::const_iterator& variable) {
            variables.erase(variable);
            variableMap.erase((*variable)->id);
        }

        /// Copy this variable list
        /// \param out the new variable list
        void CopyTo(VariableList& out) const {
            out.revision = revision;
            out.variables = variables;
            out.variableMap = variableMap;
        }

        /// Get the variable from an identifier
        /// \param id variable identifier
        /// \return nullptr if not found
        const IL::Variable* GetVariable(ID id) const {
            auto it = variableMap.find(id);
            if (it == variableMap.end()) {
                return nullptr;
            }

            return it->second;
        }

        /// Get the number of variables
        uint32_t GetCount() const {
            return static_cast<uint32_t>(variables.size());
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

        /// Variable map
        std::unordered_map<ID, const IL::Variable*> variableMap;

        /// Basic block revision
        uint32_t revision{0};
    };
}
