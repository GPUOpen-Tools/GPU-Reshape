#pragma once

// Layer
#include "DXILIDType.h"

// Backend
#include <Backend/IL/Program.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

struct DXILIDMap {
    DXILIDMap(IL::Program& program) : program(program) {

    }

    /// Copy this id map
    /// \param out destination map
    void CopyTo(DXILIDMap& out) {
        out.segment = segment;
        out.bound = bound;
    }

    /// Allocate a new mapped identifier for program <-> dxil mapping 
    /// \param type type of this mapping
    /// \param dataIndex internal data index 
    /// \return backend id
    IL::ID AllocMappedID(DXILIDType type, uint32_t dataIndex = ~0u) {
        // New unmapped?
        if (segment.allocationOffset == segment.map.Size()) {
            IL::ID id = program.GetIdentifierMap().AllocID();

            segment.map.Add(NativeState {
                .mapped = id,
                .type = type,
                .dataIndex = dataIndex
            });

            // Increment
            segment.allocationOffset++;
            bound++;

            // OK 
            return id;
        } else {
            NativeState& state = segment.map[segment.allocationOffset++];

            // May be forward declared, don't stomp it!
            if (state.mapped == IL::InvalidID) {
                IL::ID id = program.GetIdentifierMap().AllocID();

                state = NativeState {
                    .mapped = id,
                    .type = type,
                    .dataIndex = dataIndex
                };
            } else {
                // Replace type and data index with non-opaque ones
                state.type = type;
                state.dataIndex = dataIndex;
            }

            // OK
            return state.mapped;
        }
    }

    /// Remap an allocated id
    void SetMapped(uint32_t anchor, IL::ID id) {
        segment.map[anchor].mapped = id;
    }

    /// Reserve forward allocations
    /// \param count number of allocations
    void ReserveForward(size_t count) {
        size_t end = segment.map.Size() + count;

        size_t size = segment.map.Size();
        segment.map.Resize(end);

        std::fill(segment.map.begin() + size, segment.map.begin() + end, NativeState {
            .mapped = IL::InvalidID
        });

        // Increment bound
        bound += count;
    }

    /// Get the current record anchor
    uint32_t GetAnchor() const {
        return segment.allocationOffset;
    }

    /// Is an ID mapped?
    bool IsMapped(uint32_t id) const {
        return segment.allocationOffset > id;
    }

    /// Get the relative id
    /// \param anchor record anchor
    /// \param id relative id
    /// \return absolute id
    uint32_t GetRelative(uint32_t anchor, uint32_t id) const {
        return anchor - id;
    }

    /// Get the mapped relative id
    /// \param anchor record anchor
    /// \param id relative id
    /// \return absolute id
    uint32_t GetMappedRelative(uint32_t anchor, uint32_t id) const {
        ASSERT(id <= anchor, "Mapped relative refers to a forward reference, forward resolves must be handled through GetMappedForward");
        return GetMapped(anchor - id);
    }

    /// Check if a given value is resolved or not
    /// \param anchor source anchor
    /// \param id relative id
    /// \return true if resolved
    bool IsResolved(uint32_t anchor, uint32_t id) const {
        return id <= anchor;
    }

    /// Get a mapped value
    IL::ID GetMapped(uint64_t id) const {
        return segment.map[id].mapped;
    }

    /// Get a mapped value
    IL::ID GetMappedCheckType(uint64_t id, DXILIDType type) const {
        ASSERT(segment.map[id].type == type, "Unexpected type");
        return segment.map[id].mapped;
    }

    /// Get a forward mapped value
    /// \param anchor record anchor
    /// \param id forward id
    /// \return absolute id
    IL::ID GetMappedForward(uint32_t anchor, uint32_t id) {
        NativeState& state = segment.map[anchor + id];

        if (state.mapped == IL::InvalidID) {
            state = NativeState {
                .mapped = program.GetIdentifierMap().AllocID(),
                .type = DXILIDType::Forward,
                .dataIndex = 0
            };
        }

        return state.mapped;
    }

    /// Get the type of an id
    DXILIDType GetType(uint64_t id) {
        return segment.map[id].type;
    }

    /// Get the internal data index of an id
    IL::ID GetDataIndex(uint64_t id) {
        return segment.map[id].dataIndex;
    }

    /// Get the allocation bound
    uint32_t GetBound() const {
        return static_cast<uint32_t>(bound);
    }

public:
    /// Single state
    struct NativeState {
        /// Program ID
        IL::ID mapped;

        /// Identifier type
        DXILIDType type;

        /// Internal data index
        uint32_t dataIndex;
    };

    /// Snapshot of the map
    struct Snapshot {
        /// Current allocation offset
        uint32_t allocationOffset{0};

        /// Size of map
        uint64_t mapOffset{0};
    };

    /// Extracted segment of the map
    struct Segment {
        /// Snapshot used for segment branching
        Snapshot head;
        
        /// Current allocation offset
        uint32_t allocationOffset{0};

        /// All mappings
        TrivialStackVector<NativeState, 256> map;
    };

    /// Create a new snapshot, signifies a point in time for id mapping
    /// \return snapshot
    Snapshot CreateSnapshot() {
        return Snapshot {
            .allocationOffset = segment.allocationOffset,
            .mapOffset = segment.map.Size()
        };
    }

    /// Branch from a given snapshot
    /// \param from snapshot to be branched from
    /// \return segment
    Segment Branch(const Snapshot& from) {
        Segment remote;
        remote.head = from;

        // Base offset
        remote.allocationOffset = segment.allocationOffset;

        // Move allocation map
        ASSERT(segment.map.Size() >= from.mapOffset, "Remote snapshot ahead of root");
        remote.map.Resize(segment.map.Size() - from.mapOffset);
        std::memcpy(remote.map.Data(), segment.map.Data() + from.mapOffset, sizeof(NativeState) * remote.map.Size());

        // Set current state
        segment.allocationOffset = from.allocationOffset;
        segment.map.Resize(from.mapOffset);

        // OK!
        return remote;
    }

    /// Revert to a snapshot
    /// \param from snapshot to be reverted to
    void Revert(const Snapshot& from) {
        segment.allocationOffset = from.allocationOffset;
        segment.map.Resize(from.mapOffset);
    }

    /// Merge a branch
    /// \param remote branch to be merged from, heads must match
    void Merge(const Segment& remote) {
        ASSERT(segment.allocationOffset == remote.head.allocationOffset, "Invalid remote, allocation offset mismatch");
        ASSERT(segment.map.Size() == remote.head.mapOffset, "Invalid remote, map length mismatch");

        // Set new offset
        segment.allocationOffset = remote.allocationOffset;

        // Move entries
        segment.map.Resize(remote.head.mapOffset + remote.map.Size());
        std::memcpy(segment.map.Data() + remote.head.mapOffset, remote.map.Data(), sizeof(NativeState) * remote.map.Size());
    }

private:
    IL::Program& program;

private:
    /// Root segment
    Segment segment;

    /// Number of bound allocations
    uint64_t bound{0};
};
