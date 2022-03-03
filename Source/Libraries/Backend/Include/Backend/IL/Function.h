#pragma once

// Std
#include <vector>

// Backend
#include "BasicBlock.h"
#include "IdentifierMap.h"
#include "FunctionFlag.h"
#include "BasicBlockList.h"
#include "VariableList.h"
#include "Type.h"

// Std
#include <list>
#include <map>

namespace IL {
    struct Function {
        Function(const Allocators &allocators, IdentifierMap &map, ID id) :
            allocators(allocators), map(map), id(id),
            basicBlocks(allocators, map),
            parameters(allocators, map) {
            /* */
        }

        /// Allow move
        Function(Function &&other) = default;

        /// No copy
        Function(const Function &other) = delete;

        /// No copy assignment
        Function &operator=(const Function &other) = delete;
        Function &operator=(Function &&other) = delete;

        /// Copy this function
        /// \param copyMap the new identifier map
        Function Copy(IdentifierMap &copyMap) const {
            Function function(allocators, copyMap, id);
            function.sourceSpan = sourceSpan;
            function.flags = flags;
            function.functionType = functionType;

            // Copy all lists
            basicBlocks.CopyTo(function.basicBlocks);
            parameters.CopyTo(function.parameters);

            // OK
            return function;
        }

        /// Attempt to reorder all blocks by their dominant usage
        /// \return success state
        bool ReorderByDominantBlocks();

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

        /// Immortalize this function
        void Immortalize(SourceSpan span) {
            sourceSpan = span;
        }

        /// Get the number of blocks
        BasicBlockList &GetBasicBlocks() {
            return basicBlocks;
        }

        /// Get the number of blocks
        const BasicBlockList &GetBasicBlocks() const {
            return basicBlocks;
        }

        /// Get the number of blocks
        VariableList &GetParameters() {
            return parameters;
        }

        /// Get the number of blocks
        const VariableList &GetParameters() const {
            return parameters;
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

        /// Set the type of the function
        void SetFunctionType(const Backend::IL::FunctionType* type) {
            functionType = type;
        }

        /// Get the type of the function
        const Backend::IL::FunctionType* GetFunctionType() const {
            return functionType;
        }

    private:
        Allocators allocators;

        /// The source span of this function
        SourceSpan sourceSpan;

        /// Id of this function
        ID id;

        /// The shared identifier map
        IdentifierMap &map;

        /// Basic blocks
        BasicBlockList basicBlocks;

        /// All parameters
        VariableList parameters;

        /// Function type
        const Backend::IL::FunctionType* functionType{nullptr};

        /// Function flags
        FunctionFlagSet flags{0};
    };
}
