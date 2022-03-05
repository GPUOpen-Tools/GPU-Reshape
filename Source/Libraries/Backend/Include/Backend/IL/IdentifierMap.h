#pragma once

// Backend
#include "OpaqueInstructionRef.h"

// Common
#include <Common/Assert.h>

// Std
#include <list>
#include <algorithm>

namespace IL {
    struct IdentifierMap {
        using BlockUserList = std::vector<OpaqueInstructionRef>;

        /// Allocate a new ID
        /// \return
        ID AllocID() {
            map.emplace_back();
            return static_cast<ID>(map.size() - 1);
        }

        /// Set the number of bound ids
        /// \param id the capacity
        void SetBound(uint32_t bound) {
            if (map.size() > bound) {
                return;
            }

            map.resize(bound);
        }

        /// Get the maximum id
        ID GetMaxID() const {
            return static_cast<ID>(map.size());
        }

        /// Add a mapped instruction
        /// \param ref the opaque reference, must be mutable
        /// \param result the resulting id
        void AddInstruction(const OpaqueInstructionRef& ref, ID result) {
            ASSERT(result != InvalidID, "Mapping instruction with invalid result");
            map.at(result) = ref;
        }

        /// Remove a mapped instruction
        /// \param result the resulting id
        void RemoveInstruction(ID result) {
            ASSERT(result != InvalidID, "Unmapping instruction with invalid result");
            map.at(result) = {};
        }

        /// Get a mapped instruction
        /// \param id the resulting id
        /// \return may be invalid if not mapped
        const OpaqueInstructionRef& Get(const ID& id) const {
            return map.at(id);
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
            (void)std::remove(GetBlock(blockId).users.begin(), GetBlock(blockId).users.end(), user);
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
        std::vector<OpaqueInstructionRef> map;
    };
}
