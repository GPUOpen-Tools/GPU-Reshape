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

// Layer
#include <Backends/DX12/Allocation/Allocation.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/StateSubObjectWriter.h>
#include <Backends/DX12/Compiler/DXBC/DXBCExport.h>

// Common
#include <Common/Containers/Vector.h>
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Allocators.h>

// Std
#include <string>
#include <unordered_map>
#include <set>

/// Forward declarations
struct SBTIdentifierTableEntry;
struct SBTIdentifierPatch;
struct ShaderState;

struct StateObjectShaderIdentifierTableEntry {
    /// Starting list offset of this entry
    uint32_t start{UINT32_MAX};

    /// Ending list offset of this entry
    uint32_t end{0};
};

struct StateObjectShaderIdentifierTable {
    /// First stage lookup on the identifiers based on their hash
    /// Stores Start/End to the List
    StateObjectShaderIdentifierTableEntry* table{nullptr};

    /// Number of table entries
    uint64_t tableCount{0};
    
    /// Linear set of SBTIdentifierTableEntry, indexed by identifierTable
    SBTIdentifierTableEntry* list{nullptr};

    /// Allocations
    Allocation tableAllocation;
    Allocation listAllocation;
};

struct StateObjectShaderIdentifierPatch {
    /// Linear set of SBTIdentifierTableEntry, indexed by StateObjectShaderIdentifierTable
    SBTIdentifierPatch* list{nullptr};

    /// Number of patched entries
    uint64_t count{0};

    /// Allocations
    Allocation listAllocation;
};

struct StateSubObjectAssociation {
    /// Type of this association
    D3D12_STATE_SUBOBJECT_TYPE type;

    /// Inlined data payload
    TrivialStackVector<uint32_t, sizeof(uint32_t) * 4> data;
};

struct StateShaderSubObjectExport {
    std::wstring name;

    /// Scanned export
    DXBCExport dxbc;

    /// Signatures are associated with the exports themselves
    RootSignatureState* localSignature{nullptr};

    /// All inlined associations
    std::vector<StateSubObjectAssociation> associations;
};

struct StateShaderSubObject {
    /// Shader of this sub-object
    ShaderState* shader{nullptr};

    /// All exports of this sub-object
    std::vector<StateShaderSubObjectExport> functionExports;
};

struct StateSubObjectIndex {
    /// Type of the object
    D3D12_STATE_SUBOBJECT_TYPE type{};

    /// Container index
    uint32_t index{0};
};

struct __declspec(uuid("BC966B9B-874D-4707-8BD9-42784FB341CE")) StateObjectState : public PipelineState {
    StateObjectState(const Allocators &allocators) :
        PipelineState(allocators),
        shaderSubObjects(allocators),
        hitGroupSubobjects(allocators),
        writer(allocators) {
        
    }

    /// Add a new identifier patch
    /// \param hash lookup hash
    /// \param patch patch to add
    void AddPatch(uint64_t hash, StateObjectShaderIdentifierPatch* patch) {
        std::lock_guard lock(mutex);
        instrumentPatchTables[hash] = patch;
    }

    /// Get an existing identifier patch
    /// \param hash lookup hash
    /// \return nullptr if not found
    StateObjectShaderIdentifierPatch* GetPatch(uint64_t hash) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentPatchTables.find(hash);
        if (it == instrumentPatchTables.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// The type of this state object
    D3D12_STATE_OBJECT_TYPE stateObjectType{};
    
    /// The precomputed identifier table
    /// Stores general lookups
    StateObjectShaderIdentifierTable* identifierTable{nullptr};

    /// All shader subobjects
    Vector<StateShaderSubObject> shaderSubObjects;

    /// All hit groups
    Vector<D3D12_HIT_GROUP_DESC> hitGroupSubobjects;

    /// Current hot patch table
    std::atomic<StateObjectShaderIdentifierPatch*> hotSwapPatchTable{nullptr};

    /// All hot patch tables
    std::map<uint64_t, StateObjectShaderIdentifierPatch*> instrumentPatchTables;

    // TODO[rt]: Separate allocation isn't needed, subobject can hold the memory
    // TODO[rt]: This is too micro-allocation heavy

    /// All identifier exports, all callable
    std::set<std::wstring> identifierExports;

    /// Export name to sub object lookup
    std::unordered_map<std::wstring, StateSubObjectIndex> subObjectMap;

    /// The state object may inline some states, such as those that come from DXIL libraries
    std::vector<IUnknown*> inlinedSubObjectStates;

    /// Defacto deep copy for this state object
    StateSubObjectWriter writer;
};
