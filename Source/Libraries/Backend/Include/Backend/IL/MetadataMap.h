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
        static constexpr uint32_t kNoMember = UINT32_MAX;
        
        MetadataMap(const Allocators& allocators) : payloadAllocator(allocators) {
            
        }

        /// Copy this map
        /// \param out destination map
        void CopyTo(MetadataMap& out) const {
            out.entries = entries;
        }

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param id memberIndex the member index of the composite
        /// \param type given metadata
        void AddMetadata(ID id, uint32_t memberIndex, MetadataType type) {
            MetadataBucket& bucket = GetBucketOrAdd(id, memberIndex);
            
            bucket.metadatas.push_back(Metadata {
                .type = type
            });
        }

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param type given metadata
        void AddMetadata(ID id, MetadataType type) {
            AddMetadata(id, kNoMember, type);
        }

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param id memberIndex the member index of the composite
        /// \param payload payload to assign, type derived from embedded id
        template<typename T>
        void AddMetadata(ID id, uint32_t memberIndex, const T& payload) {
            MetadataBucket& bucket = GetBucketOrAdd(id, memberIndex);

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

        /// Add a new metadata specifier
        /// \param id destination id
        /// \param payload payload to assign, type derived from embedded id
        template<typename T>
        void AddMetadata(ID id, const T& payload) {
            AddMetadata<T>(id, kNoMember, payload);
        }

        /// Get a metadata payload
        /// \tparam T expected metadata type
        /// \param id id to query
        /// \param id memberIndex the member index of the composite
        /// \return nullptr if not found
        template<typename T>
        const T* GetMetadata(ID id, uint32_t memberIndex) {
            MetadataBucket* bucket = GetBucket(id, memberIndex);
            if (!bucket) {
                return nullptr;
            }
            
            for (const Metadata& metadata : bucket->metadatas) {
                if (metadata.type == T::kID) {
                    return static_cast<const T*>(metadata.payload);
                }
            }

            return nullptr;
        }

        /// Get a metadata payload
        /// \tparam T expected metadata type
        /// \param id id to query
        /// \return nullptr if not found
        template<typename T>
        const T* GetMetadata(ID id) {
            return GetMetadata<T>(id, kNoMember);
        }

        /// Check if a metadata type exists
        /// \param id id to query
        /// \param id memberIndex the member index of the composite
        /// \param type type to search for
        /// \return false if not found
        bool HasMetadata(ID id, uint32_t memberIndex, MetadataType type) {
            MetadataBucket* bucket = GetBucket(id, memberIndex);
            if (!bucket) {
                return false;
            }
            
            for (const Metadata& metadata : bucket->metadatas) {
                if (metadata.type == type) {
                    return true;
                }
            }

            return false;
        }

        /// Check if a metadata type exists
        /// \param id id to query
        /// \param type type to search for
        /// \return false if not found
        bool HasMetadata(ID id, MetadataType type) {
            return HasMetadata(id, kNoMember, type);
        }

        /// Check if a metadata type exists
        /// \tparam T payload type to query for
        /// \param id id to query
        /// \param id memberIndex the member index of the composite
        /// \return false if not found
        template<typename T>
        bool HasMetadata(ID id, uint32_t memberIndex) const {
            return HasMetadata(id, memberIndex, T::kID);
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

        struct ValueEntry {
            /// Value bucket
            MetadataBucket value;

            /// Optional member wise buckets
            std::vector<MetadataBucket> members;
        };

        MetadataBucket* GetBucket(ID id, uint32_t memberIndex) {
            auto entryIt = entries.find(id);
            if (entryIt == entries.end()) {
                return nullptr;
            }

            // If not a member, just return the value
            if (memberIndex == kNoMember) {
                return &entryIt->second.value;
            }

            // Any member bucket?
            if (memberIndex >= entryIt->second.members.size()) {
                return nullptr;
            }

            return &entryIt->second.members[memberIndex];
        }

        MetadataBucket& GetBucketOrAdd(ID id, uint32_t memberIndex) {
            ValueEntry& entry = entries[id];

            // If not a member, just return the value
            if (memberIndex == kNoMember) {
                return entry.value;
            }

            // If no member bucket, create it
            if (memberIndex >= entry.members.size()) {
                entry.members.resize(memberIndex + 1);
            }

            return entry.members[memberIndex];
        }

    private:
        /// All id-wise buckets
        std::unordered_map<ID, ValueEntry> entries;

        /// Underlying allocator
        LinearBlockAllocator<1024> payloadAllocator;
    };
}
