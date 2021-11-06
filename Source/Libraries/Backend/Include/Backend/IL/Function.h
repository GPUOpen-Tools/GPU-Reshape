#pragma once

// Std
#include <vector>

// Backend
#include "BasicBlock.h"

// Std
#include <list>

namespace IL {
    struct Function {
        Function(ID id) : id(id) {

        }

        /// Allocate a new basic block
        /// \return
        BasicBlock* AllocBlock(ID id) {
            basicBlocks.emplace_back(id);
            return &basicBlocks.back();
        }

        /// Get the number of blocks
        uint32_t GetBlockCount() const {
            return static_cast<uint32_t>(basicBlocks.size());
        }

        /// Get the id of this function
        ID GetID() const {
            return id;
        }

        std::list<BasicBlock>::iterator begin() {
            return basicBlocks.begin();
        }

        std::list<BasicBlock>::iterator end() {
            return basicBlocks.end();
        }

        std::list<BasicBlock>::const_iterator begin() const {
            return basicBlocks.begin();
        }

        std::list<BasicBlock>::const_iterator end() const {
            return basicBlocks.end();
        }

    private:
        /// Id of this function
        ID id;

        /// Basic blocks
        std::list<BasicBlock> basicBlocks;
    };
}
