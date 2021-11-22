#pragma once

// Std
#include <vector>

// Backend
#include "BasicBlock.h"
#include "IdentifierMap.h"

// Std
#include <list>

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

            // Copy all basic blocks
            for (const BasicBlock& bb : basicBlocks) {
                function.basicBlocks.emplace_back(bb.Copy(copyMap));
            }

            return function;
        }

        /// Allocate a new basic block
        /// \return
        BasicBlock* AllocBlock(ID bid) {
            basicBlocks.emplace_back(allocators, std::ref(map), bid);
            return &basicBlocks.back();
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

        /// Get the source span of this function
        SourceSpan GetSourceSpan() const {
            return sourceSpan;
        }

        /// Get the declaration source span
        SourceSpan GetDeclarationSourceSpan() const {
            return {sourceSpan.begin, sourceSpan.begin};
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
    };
}
