#pragma once

// Std
#include <list>
#include <map>

// Backend
#include "BasicBlock.h"

namespace IL {
    struct BasicBlockList {
        using Container = std::list<BasicBlock>;

        BasicBlockList(const Allocators& allocators, IdentifierMap& map) : allocators(allocators), map(map) {

        }

        /// Allocate a new basic block
        /// \return allocated basic block
        BasicBlock* AllocBlock(ID bid) {
            revision++;

            BasicBlock& bb = basicBlocks.emplace_back(allocators, std::ref(map), bid);
            blockMap[bid] = &bb;
            return &bb;
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
            auto it = blockMap.find(bid);
            if (it == blockMap.end()) {
                return nullptr;
            }

            return it->second;
        }

        /// Get the current basic block revision
        uint32_t GetBasicBlockRevision() const {
            return revision;
        }

        /// Remove a basic block
        /// \param block iterator
        void Remove(const Container::const_iterator& block) {
            blockMap[block->GetID()] = nullptr;
            basicBlocks.erase(block);
        }

        /// Add (move) a basic block
        /// \param block block to be moved
        void Add(BasicBlock&& block) {
            auto& moved = basicBlocks.emplace_back(std::move(block));
            blockMap[moved.GetID()] = &moved;
        }

        /// Swap blocks with a container
        /// \param list container to be swapped with
        void SwapBlocks(Container& list) {
            basicBlocks.swap(list);
            blockMap.clear();
        }

        /// Get the number of blocks
        uint32_t GetBlockCount() const {
            return static_cast<uint32_t>(basicBlocks.size());
        }

        /// Copy this function
        /// \param copyMap the new identifier map
        void CopyTo(BasicBlockList& out) const {
            out.revision = revision;

            // Copy all basic blocks
            for (const BasicBlock& bb : basicBlocks) {
                BasicBlock& copy = out.basicBlocks.emplace_back(bb.Copy(out.map));
                out.blockMap[copy.GetID()] = &copy;
            }
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
        std::map<ID, BasicBlock*> blockMap;

        /// Basic block revision
        uint32_t revision{0};
    };
}
