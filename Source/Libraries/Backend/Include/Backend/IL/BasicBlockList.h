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
#include <unordered_map>

// Backend
#include "BasicBlock.h"

namespace IL {
    struct BasicBlockList {
        using Container = std::vector<BasicBlock*>;

        BasicBlockList(const Allocators& allocators, IdentifierMap& map) : allocators(allocators), map(map) {

        }

        /// Destructor
        ~BasicBlockList() {
            for (BasicBlock* block: basicBlocks) {
                destroy(block, allocators);
            }
        }

        /// Allocate a new basic block
        /// \return allocated basic block
        BasicBlock* AllocBlock(ID bid) {
            revision++;

            auto* basicBlock = new (allocators) BasicBlock(allocators, map, bid);
            basicBlocks.push_back(basicBlock);
            basicBlockMap[bid] = basicBlock;
            return basicBlock;
        }

        /// Allocate a new basic block
        /// \return allocated basic block
        BasicBlock* AllocBlock() {
            return AllocBlock(map.AllocID());
        }

        /// Get a block from an identifier
        /// \param bid basic block identifier
        /// \return nullptr if not found
        BasicBlock* GetBlock(ID bid) const {
            auto it = basicBlockMap.find(bid);
            if (it == basicBlockMap.end()) {
                return nullptr;
            }

            return it->second;
        }

        /// Get the current basic block revision
        uint32_t GetBasicBlockRevision() const {
            return revision;
        }

        /// Remove a basic block
        /// \param block block to be removed
        void Remove(BasicBlock* block) {
            basicBlockMap.erase(block->GetID());

            // Note: Removing by value is somewhat acceptable, we are not expecting large amounts of blocks
            basicBlocks.erase(std::find(basicBlocks.begin(), basicBlocks.end(), block));
        }

        /// Add a basic block
        /// \param block block to be added
        void Add(BasicBlock* block) {
            basicBlocks.push_back(block);
            basicBlockMap[block->GetID()] = block;
        }

        /// Rename an existing block
        /// \param block block to be added
        /// \param id block to be added
        void RenameBlock(BasicBlock* block, IL::ID id) {
            ASSERT(basicBlockMap.contains(block->GetID()), "Renaming a block of foreign residence");

            Remove(block);
            block->SetID(id);
            Add(block);
        }

        /// Swap blocks with a container
        /// \param list container to be swapped with
        void SwapBlocks(Container& list) {
            basicBlocks.swap(list);
            basicBlockMap.clear();
        }

        /// Get the number of blocks
        uint32_t GetBlockCount() const {
            return static_cast<uint32_t>(basicBlocks.size());
        }

        /// Get the maximum block id
        uint32_t GetBlockMaxID() const {
            uint32_t id = 0;

            // Get max id
            for (BasicBlock* bb : basicBlocks) {
                id = std::max(id, bb->GetID());
            }

            return id;
        }

        /// Get the maximum block id
        uint32_t GetBlockBound() const {
            return GetBlockMaxID() + 1u;
        }

        /// Copy this basicBlock
        /// \param copyMap the new identifier map
        void CopyTo(BasicBlockList& out) const {
            out.revision = revision;

            // Copy all basic blocks
            for (const BasicBlock* bb : basicBlocks) {
                auto* copy = new (allocators) BasicBlock(allocators, out.map, bb->GetID());
                bb->CopyTo(copy);

                out.basicBlocks.push_back(copy);
                out.basicBlockMap[copy->GetID()] = copy;
            }
        }

        /// Get the entry point block
        IL::BasicBlock* GetEntryPoint() const {
            return basicBlocks[0];
        }

        /// Iterator accessors
        Container::iterator begin() { return basicBlocks.begin(); }
        Container::reverse_iterator rbegin() { return basicBlocks.rbegin(); }
        Container::iterator end() { return basicBlocks.end(); }
        Container::reverse_iterator rend() { return basicBlocks.rend(); }
        Container::const_iterator begin() const { return basicBlocks.begin(); }
        Container::const_reverse_iterator rbegin() const { return basicBlocks.rbegin(); }
        Container::const_iterator end() const { return basicBlocks.end(); }
        Container::const_reverse_iterator rend() const { return basicBlocks.rend(); }

    private:
        Allocators allocators;

        /// The shared identifier map
        IdentifierMap& map;

        /// All blocks
        Container basicBlocks;

        /// Block map
        std::unordered_map<ID, BasicBlock*> basicBlockMap;

        /// Basic block revision
        uint32_t revision{0};
    };
}
