#pragma once

// Std
#include <vector>

// Backend
#include "BasicBlock.h"
#include "IdentifierMap.h"
#include "FunctionFlag.h"

// Std
#include <list>
#include <map>

namespace IL {
    struct Function {
        Function(const Allocators& allocators, IdentifierMap& map, ID id) : allocators(allocators), map(map), id(id) {

        }

        /// Allow move
        Function(Function&& other) = default;

        /// No copy
        Function(const Function& other) = delete;

        /// No copy assignment
        Function& operator=(const Function& other) = delete;
        Function& operator=(Function&& other) = delete;

        /// Copy this function
        /// \param copyMap the new identifier map
        Function Copy(IdentifierMap& copyMap) const {
            Function function(allocators, copyMap, id);
            function.sourceSpan = sourceSpan;
            function.basicBlockRevision = basicBlockRevision;
            function.flags = flags;

            // Copy all basic blocks
            for (const BasicBlock& bb : basicBlocks) {
                function.basicBlocks.emplace_back(bb.Copy(copyMap));
            }

            return function;
        }

        /// Add a new flag to this function
        /// \param value flag to be added
        void AddFlag(FunctionFlagSet value) {
            flags |= value;
        }

        /// Remove a flag from this function
        /// \param value flag to be removed
        void RemoveFlag(FunctionFlagSet value) {
            flags &= ~value;
        }

        /// Check if this function has a flag
        /// \param value flag to be checked
        /// \return true if present
        bool HasFlag(FunctionFlag value) {
            return flags & value;
        }

        /// Allocate a new basic block
        /// \return allocated basic block
        BasicBlock* AllocBlock(ID bid) {
            basicBlockRevision++;

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

        /// Immortalize this function
        void Immortalize(SourceSpan span) {
            sourceSpan = span;
        }

        /// Get the number of blocks
        uint32_t GetBlockCount() const {
            return static_cast<uint32_t>(basicBlocks.size());
        }

        /// Get the id of this function
        ID GetID() const {
            return id;
        }

        /// Get all flags
        FunctionFlagSet GetFlags() const {
            return flags;
        }

        /// Get the source span of this function
        SourceSpan GetSourceSpan() const {
            return sourceSpan;
        }

        /// Get the declaration source span
        SourceSpan GetDeclarationSourceSpan() const {
            return {sourceSpan.begin, sourceSpan.begin};
        }

        /// Get the current basic block revision
        uint32_t GetBasicBlockRevision() const {
            return basicBlockRevision;
        }

        /// Iterator
        std::list<BasicBlock>::iterator begin() {
            return basicBlocks.begin();
        }

        /// Iterator
        std::list<BasicBlock>::reverse_iterator rbegin() {
            return basicBlocks.rbegin();
        }

        /// Iterator
        std::list<BasicBlock>::iterator end() {
            return basicBlocks.end();
        }

        /// Iterator
        std::list<BasicBlock>::reverse_iterator rend() {
            return basicBlocks.rend();
        }

        /// Iterator
        std::list<BasicBlock>::const_iterator begin() const {
            return basicBlocks.begin();
        }

        /// Iterator
        std::list<BasicBlock>::const_reverse_iterator rbegin() const {
            return basicBlocks.rbegin();
        }

        /// Iterator
        std::list<BasicBlock>::const_iterator end() const {
            return basicBlocks.end();
        }

        /// Iterator
        std::list<BasicBlock>::const_reverse_iterator rend() const {
            return basicBlocks.rend();
        }

    private:
        Allocators allocators;

        /// The source span of this function
        SourceSpan sourceSpan;

        /// Id of this function
        ID id;

        /// The shared identifier map
        IdentifierMap& map;

        /// Basic blocks
        std::list<BasicBlock> basicBlocks;

        /// Basic block revision
        uint32_t basicBlockRevision{0};

        /// Block map
        std::map<ID, BasicBlock*> blockMap;

        /// Function flags
        FunctionFlagSet flags{0};
    };
}
