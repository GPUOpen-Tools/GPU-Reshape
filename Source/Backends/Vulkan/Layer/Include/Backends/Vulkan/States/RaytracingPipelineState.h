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
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/Allocation/Allocation.h>

// Std
#include <unordered_map>
#include <span>

// Forward declarations
struct SBTIdentifierTableEntry;
struct SBTIdentifierPatch;

struct RaytracingShaderGroupIdentifierKey {
    /// Equality check
    bool operator==(const RaytracingShaderGroupIdentifierKey& other) const {
        if (hash != other.hash) {
            return false;
        }

        // Always double check handle data
        return !std::memcmp(data.data(), other.data.data(), data.size());
    }

    /// Hash of this identifier
    uint32_t hash = UINT32_MAX;

    /// Underlying data, not owned
    std::span<const uint8_t> data;
};

template <>
struct std::hash<RaytracingShaderGroupIdentifierKey> {
    std::size_t operator()(const RaytracingShaderGroupIdentifierKey& value) const {
        return value.hash;
    }
};

struct RaytracingShaderGroupIdentifierSet {
    /// Number of identifiers, includes libraries
    uint32_t count = 0;

    /// All identifier data, excluding embedded
    std::vector<uint8_t> handleData;

    /// All identifier to patch index lookups
    std::unordered_map<RaytracingShaderGroupIdentifierKey, uint32_t> patchIndices;
};

struct RaytracingShaderIdentifierPatch {
    /// Linear set of handle data
    uint8_t* patchHandleData{nullptr};

    /// Allocations
    Allocation listAllocation;

    /// Host buffer
    VkBuffer buffer{VK_NULL_HANDLE};
};

struct RaytracingPipelineState : public PipelineState {
    /// Add a new identifier patch
    /// \param hash lookup hash
    /// \param patch patch to add
    void AddPatch(uint64_t hash, RaytracingShaderIdentifierPatch* patch) {
        std::lock_guard lock(mutex);
        instrumentPatchTables[hash] = patch;
    }

    /// Get an existing identifier patch
    /// \param hash lookup hash
    /// \return nullptr if not found
    RaytracingShaderIdentifierPatch* GetPatch(uint64_t hash) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentPatchTables.find(hash);
        if (it == instrumentPatchTables.end()) {
            return nullptr;
        }

        return it->second;
    }
    
    /// Deep copy
    VkRayTracingPipelineCreateInfoKHRDeepCopy createInfoDeepCopy;

    /// Identifier set for the full pipeline, includes libraries
    RaytracingShaderGroupIdentifierSet identifierSet;

    /// Current hot patch table
    std::atomic<RaytracingShaderIdentifierPatch*> hotSwapPatchTable{nullptr};

    /// All hot patch tables
    std::map<uint64_t, RaytracingShaderIdentifierPatch*> instrumentPatchTables;
};
