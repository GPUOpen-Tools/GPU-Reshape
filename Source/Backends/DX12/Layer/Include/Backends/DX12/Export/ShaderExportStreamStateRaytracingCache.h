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
#include <Backends/DX12/DX12.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <vector>

// Forward declarations
struct ResourceState;

struct ShaderExportStreamStateRaytracingPatchEntry {
    /// Check if this entry references a resource
    bool References(ResourceState* state) const {
        for (ResourceState* resource : resources) {
            if (state == resource) {
                return true;
            }
        }

        // Not referenced
        return false;
    }

    /// Hash of this entry
    uint64_t hash{0};
    
    /// Patched description, lifetime tied to the stream state's lifetime
    D3D12_DISPATCH_RAYS_DESC patched;

    /// All referenced resources
    TrivialStackVector<ResourceState*, 4u> resources;
};

struct ShaderExportStreamStateRaytracingCache {
    /// Clear all cached state
    void Clear() {
        patchEntries.clear();
    }

    /// Invalidate all state for a particular resource
    /// \param state state to invalidate for
    void Invalidate(ResourceState* state) {
        // Remove all entries that reference the state
        for (int64_t i = patchEntries.size() - 1; i >= 0; i--) {
            if (patchEntries[i].References(state)) {
                patchEntries.erase(patchEntries.begin() + i);
            }
        }
    }

    /// All patched dispatches
    std::vector<ShaderExportStreamStateRaytracingPatchEntry> patchEntries;
};
