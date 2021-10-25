#pragma once

// Std
#include <vector>

// Backend
#include "Instruction.h"

namespace IL {
    struct BasicBlock {
        template<typename T>
        void Insert(const T& instr) {
            size_t offset = data.size();
            data.resize(data.size() + sizeof(T));
            std::memcpy(&data[offset], &instr, sizeof(T));
        }

    private:
        std::vector<uint8_t> data;
    };
}
