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

// Backend
#include "OpaqueInstructionRef.h"

// Common
#include <Common/Assert.h>

// Std
#include <list>
#include <algorithm>
#include <unordered_map>

namespace IL {
    struct IdentifierMap {
        using BlockUserList = std::vector<OpaqueInstructionRef>;

        /// Allocate a new ID
        /// \return
        ID AllocID() {
            instructionMap.emplace_back();
            return static_cast<ID>(instructionMap.size() - 1);
        }

        /// Allocate a range of IDs
        /// \return base ID
        ID AllocIDRange(uint32_t count) {
            auto base = static_cast<ID>(instructionMap.size());
            instructionMap.resize(instructionMap.size() + count);
            return base;
        }

        /// Set the number of bound ids
        /// \param id the capacity
        void SetBound(uint32_t bound) {
            if (instructionMap.size() > bound) {
                return;
            }

            instructionMap.resize(bound);
        }

        /// Get the maximum id
        ID GetMaxID() const {
            return static_cast<ID>(instructionMap.size());
        }

        /// Add a mapped instruction
        /// \param ref the opaque reference, must be mutable
        /// \param result the resulting id
        void AddInstruction(const OpaqueInstructionRef& ref, ID result) {
            ASSERT(result != InvalidID, "Mapping instruction with invalid result");
            instructionMap.at(result) = ref;
        }

        /// Remove a mapped instruction
        /// \param result the resulting id
        void RemoveInstruction(ID result) {
            ASSERT(result != InvalidID, "Unmapping instruction with invalid result");
            instructionMap.at(result) = {};
        }

        /// Redirect an instruction
        /// This has no semantic relevance except for programs which explicitly fetch source instructions
        /// \param original the original instruction result
        /// \param redirect the new / redirected result
        void RedirectInstruction(ID original, ID redirect) {
            ASSERT(original != InvalidID && redirect != InvalidID, "Unmapping instruction with invalid result");

            // Lazy allocate, most instrumentation will not require redirects
            if (redirect >= redirectMap.size()) {
                redirectMap.resize(redirect + 1, InvalidID);
            }

            // If the destination is a redirect in and of itself, is the destination's redirect
            // Ensures we always point to the source
            if (IL::ID existingRedirect = redirectMap[original]; existingRedirect != InvalidID) {
                original = existingRedirect;
            }
            
            redirectMap[redirect] = original;
        }

        /// Get the source / original instruction id from a potentially redirected id
        /// \param id the id to query
        /// \return source id (always valid)
        IL::ID GetSourceInstruction(ID id) const {
            if (id >= redirectMap.size()) {
                return id;
            }

            // Check the redirect map
            if (ID redirect = redirectMap[id]; redirect != InvalidID) {
                return redirect;
            }

            // No redirect, this is a source id
            return id;
        }

        /// Get a mapped instruction
        /// \param id the resulting id
        /// \return may be invalid if not mapped
        const OpaqueInstructionRef& Get(const ID& id) const {
            return instructionMap.at(id);
        }

        /// Add a mapped basic block
        /// \param block the block to be mapped
        /// \param result the resulting id
        void AddBasicBlock(BasicBlock* block, ID result) {
            ASSERT(result != InvalidID, "Mapping block with invalid id");
            blockMap[result] = block;
        }

        /// Remove a mapped basic block
        /// \param result the resulting id
        void RemoveBasicBlock(ID result) {
            ASSERT(result != InvalidID, "Unmapping block with invalid id");
            blockMap.at(result) = nullptr;
        }

        /// Get a basic block
        /// \param id id to fetch
        /// \return fetched block
        BasicBlock* GetBasicBlock(ID id) const {
            return blockMap.at(id);
        }

        /// Add a new user to a block
        /// \param blockId referenced block
        /// \param user user of said block
        void AddBlockUser(const ID& blockId, const OpaqueInstructionRef& user) {
            GetBlock(blockId).users.push_back(user);
        }

        /// Remove a user from a block
        /// \param blockId referenced block
        /// \param user user of said block
        void RemoveBlockUser(const ID& blockId, const OpaqueInstructionRef& user) {
            Block& block = GetBlock(blockId);
            block.users.erase(std::find(block.users.begin(), block.users.end(), user));
        }

        /// Get the users for a specific block
        /// \param id the id of the block
        /// \return user list, not mutable
        const BlockUserList& GetBlockUsers(const ID& id) {
            return GetBlock(id).users;
        }

    private:
        struct Block {
            BlockUserList users;
        };

        /// Get a specific block
        Block& GetBlock(const ID& id) {
            if (id >= blocks.size()) {
                blocks.resize(id + 1);
            }

            return blocks[id];
        }

    private:
        /// All blocks
        std::vector<Block> blocks;

        /// All instructions
        std::vector<OpaqueInstructionRef> instructionMap;

        /// All redirected identifiers
        std::vector<IL::ID> redirectMap;

        /// All basic blocks
        std::unordered_map<ID, BasicBlock*> blockMap;
    };
}
