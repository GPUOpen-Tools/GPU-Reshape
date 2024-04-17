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
#include <Backend/IL/MetadataType.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <cstdint>
#include <unordered_map>

namespace IL {
    struct MetadataMap {
        MetadataMap(const Allocators& allocators) : payloadAllocator(allocators) {
            
        }

        /// Copy this map
        /// \param out destination map
        void CopyTo(MetadataMap& out) const {
            out.buckets = buckets;
        }

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param type given metadata
        void AddMetadata(ID id, MetadataType type) {
            buckets[id].metadatas.push_back(Metadata {
                .type = type
            });
        }

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param payload payload to assign, type derived from embedded id
        template<typename T>
        void AddMetadata(ID id, const T& payload) {
            MetadataBucket& bucket = buckets[id];

            // Try to replace existing metadata
            for (Metadata& metadata : bucket.metadatas) {
                if (metadata.type == T::kID) {
                    metadata.payload = payloadAllocator.Allocate<T>(payload);
                    return;
                }
            }

            // None found, add new
            bucket.metadatas.push_back(Metadata {
                .type = T::kID,
                .payload = payloadAllocator.Allocate<T>(payload)
            });
        }

        /// Get a metadata payload
        /// \tparam T expected metadata type
        /// \param id id to query
        /// \return nullptr if not found
        template<typename T>
        const T* GetMetadata(ID id) {
            auto it = buckets.find(id);
            if (it == buckets.end()) {
                return nullptr;
            }
            
            for (const Metadata& metadata : it->second.metadatas) {
                if (metadata.type == T::kID) {
                    return static_cast<const T*>(metadata.payload);
                }
            }

            return nullptr;
        }

        /// Check if a metadata type exists
        /// \param id id to query
        /// \param type type to search for
        /// \return false if not found
        bool HasMetadata(ID id, MetadataType type) {
            auto it = buckets.find(id);
            if (it == buckets.end()) {
                return false;
            }
            
            for (const Metadata& metadata : it->second.metadatas) {
                if (metadata.type == type) {
                    return true;
                }
            }

            return false;
        }

        /// Check if a metadata type exists
        /// \tparam T payload type to query for
        /// \param id id to query
        /// \return false if not found
        template<typename T>
        bool HasMetadata(ID id) const {
            return HasMetadata(id, T::kID);
        }

    private:
        struct Metadata {
            /// Type of this metadata
            MetadataType type{MetadataType::None};

            /// Optional, payload data
            const void* payload{nullptr};
        };
        
        struct MetadataBucket {
            /// All metadata in this bucket
            std::vector<Metadata> metadatas;
        };

        /// All id-wise buckets
        std::unordered_map<ID, MetadataBucket> buckets;

        LinearBlockAllocator<1024> payloadAllocator;
    };
}
