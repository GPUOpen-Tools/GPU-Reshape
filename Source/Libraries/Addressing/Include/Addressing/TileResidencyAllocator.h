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

// Feature
#include <Addressing/TileMappingRequest.h>

// Backend
#include <Backend/ShaderData/ShaderData.h>

// Std
#include <vector>

class TileResidencyAllocator {    
public:
    /// Install this allocator
    /// \param size total byte size requested
    void Install(uint64_t size) {
        // Determine the number of tiles
        uint32_t tileCount = static_cast<uint32_t>((size + kShaderDataMappingTileWidth - 1) / kShaderDataMappingTileWidth);

        // Default to no resident pages
        tileResidency.clear();
        tileResidency.resize(tileCount, 0);
    }

    /// Allocate a region
    /// \param offset byte offset
    /// \param length byte length from the offset
    void Allocate(uint64_t offset, uint64_t length) {
        // To tile regions
        uint32_t tileOffset = static_cast<uint32_t>(offset / kShaderDataMappingTileWidth);
        uint32_t tileCount = static_cast<uint32_t>((length + kShaderDataMappingTileWidth - 1) / kShaderDataMappingTileWidth);

        // Current residency offset
        uint32_t residenceStart = 0;

        // Go through all relevant tiles
        for (uint32_t i = 0; i < tileCount; i++) {
            if (!TestOrMarkResident(tileOffset + i)) {
                continue;
            }

            // If this tile is resident, and any previously iterated are not, submit that chunk
            if (residenceStart != i) {
                requests.push_back(TileMappingRequest {
                    .tileOffset = tileOffset + residenceStart,
                    .tileCount = i - residenceStart
                });
            }

            // Starting new segment
            MarkResident(tileOffset + i);
            residenceStart = i + 1;
        }

        // Push dangling requests
        if (residenceStart != tileCount) {
            requests.push_back(TileMappingRequest {
                .tileOffset = tileOffset + residenceStart,
                .tileCount = tileCount - residenceStart
            });
        }
    }

    /// Check if a tile is resident
    /// \param tileIndex given index
    /// \return true if resident
    bool IsResident(uint32_t tileIndex) const {
        return tileResidency[tileIndex >> 5] & (1u << (tileIndex & 31));
    }

    /// Get the number of pending requests
    /// \return number of requests
    uint32_t GetRequestCount() const {
        return static_cast<uint32_t>(requests.size());
    }

    /// Get a pending request
    /// \param i request index
    /// \return request
    const TileMappingRequest& GetRequest(uint32_t i) const {
        return requests[i];
    }

    /// Clear all pending requests
    void ClearRequests() {
        requests.clear();
    }

private:
    /// Mark a given tile as resident
    /// \param tileIndex index
    void MarkResident(uint32_t tileIndex) {
        tileResidency[tileIndex >> 5] |= 1u << (tileIndex & 31);
    }
    /// Mark a tile as resident and return if it 
    /// \param tileIndex index
    bool TestOrMarkResident(uint32_t tileIndex) {
        uint32_t bit = 1u << (tileIndex & 31);
        
        const bool isResident = tileResidency[tileIndex >> 5] & bit;
        tileResidency[tileIndex >> 5] |= bit;
        return isResident;
    }

private:
    /// All tiles
    std::vector<uint32_t> tileResidency;

    /// Pending mapping requests
    std::vector<TileMappingRequest> requests;
};
