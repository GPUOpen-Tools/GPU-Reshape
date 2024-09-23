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
#include <vector>

// Backend
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/IdentifierMap.h>
#include <Backend/IL/FunctionFlag.h>
#include <Backend/IL/BasicBlockList.h>
#include <Backend/IL/VariableList.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/Analysis/AnalysisMap.h>

// Std
#include <list>
#include <map>

namespace IL {
    struct Function {
        Function(const Allocators &allocators, IdentifierMap &map, ID id) :
            allocators(allocators), id(id), map(map),
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
        void CopyTo(Function* out) const {
            out->sourceSpan = sourceSpan;
            out->flags = flags;
            out->functionType = functionType;

            // Copy all lists
            basicBlocks.CopyTo(out->basicBlocks);
            parameters.CopyTo(out->parameters);
        }

        /// Reindex all users
        void IndexUsers() const {
            for (IL::BasicBlock* bb : basicBlocks) {
                bb->IndexUsers();
            }
        }

        /// Attempt to reorder all blocks by their dominant usage
        /// \return success state
        bool ReorderByDominantBlocks(bool hasControlFlow);

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

        /// Get the analysis map
        AnalysisMap<IFunctionAnalysis> &GetAnalysisMap() {
            return analysisMap;
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
        void SetFunctionType(const IL::FunctionType* type) {
            functionType = type;
        }

        /// Get the type of the function
        const IL::FunctionType* GetFunctionType() const {
            return functionType;
        }

        /// Get the identifier map
        IdentifierMap &GetIdentifierMap() {
            return map;
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

        /// All analysis passes
        AnalysisMap<IFunctionAnalysis> analysisMap;

        /// Function type
        const IL::FunctionType* functionType{nullptr};

        /// Function flags
        FunctionFlagSet flags{0};
    };
}
