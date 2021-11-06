#pragma once

// Backend
#include "Function.h"

// Std
#include <list>
#include <algorithm>

namespace IL {
    struct Program {
        /// Allocate a new ID
        /// \return
        ID AllocID() {
            return counter++;
        }

        /// Set the number of bound ids
        /// \param id the capacity
        void SetBound(uint32_t bound) {
            counter = std::max(counter, bound);
        }

        /// Allocate a new basic block
        /// \return
        Function* AllocFunction(ID id) {
            functions.emplace_back(id);
            return &functions.back();
        }

        /// Get the maximum id
        ID GetMaxID() const {
            return counter;
        }

        /// Get the number of functions
        uint32_t GetFunctionCount() const {
            return static_cast<uint32_t>(functions.size());
        }

        std::list<Function>::iterator begin() {
            return functions.begin();
        }

        std::list<Function>::iterator end() {
            return functions.end();
        }

        std::list<Function>::const_iterator begin() const {
            return functions.begin();
        }

        std::list<Function>::const_iterator end() const {
            return functions.end();
        }

    private:
        /// Functions within this program
        std::list<Function> functions;

        /// Id allocation counter
        ID counter{0};
    };
}
