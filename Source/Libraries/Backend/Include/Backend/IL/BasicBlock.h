#pragma once

// Std
#include <vector>

// Backend
#include "Instruction.h"

namespace IL {
    struct BasicBlock {
        BasicBlock(ID id) : id(id) {

        }

        /// Insert an instruction
        template<typename T>
        void Insert(const T& instr) {
            size_t offset = data.size();
            data.resize(data.size() + sizeof(T));
            std::memcpy(&data[offset], &instr, sizeof(T));
            count++;
            dirty = true;
        }

        /// Check if this basic block has been modified
        bool IsModified() const {
            return dirty;
        }

        /// Immortalize this basic block
        void Immortalize() {
            dirty = false;
        }

        /// Get the number of instructions
        uint32_t GetCount() const {
            return count;
        }

        /// Get the id of this basic block
        ID GetID() const {
            return id;
        }

    private:
        /// Label id
        ID id;

        /// Number of instructions
        uint32_t count{0};

        /// Instruction stream
        std::vector<uint8_t> data;

        /// Dirty flag
        bool dirty{true};
    };
}
