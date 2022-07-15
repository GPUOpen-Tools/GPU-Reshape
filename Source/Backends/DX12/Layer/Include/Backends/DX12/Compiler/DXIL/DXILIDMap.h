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

    /// Allocate a new mapped identifier for program <-> dxil mapping
    /// \param type type of this mapping
    /// \param dataIndex internal data index
    /// \return backend id
    IL::ID AllocMappedID(DXILIDType type, uint32_t dataIndex = ~0u) {
        // New unmapped?
        if (allocationOffset == map.Size()) {
            IL::ID id = program.GetIdentifierMap().AllocID();

            map.Add(NativeState {
                .mapped = id,
                .type = type,
                .dataIndex = dataIndex
            });

            // Increment
            allocationOffset++;

            // OK
            return id;
        } else {
            NativeState& state = map[allocationOffset++];

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
        map[anchor].mapped = id;
    }

    /// Reserve forward allocations
    /// \param count number of allocations
    void ReserveForward(size_t count) {
        size_t end = map.Size() + count;

        size_t size = map.Size();
        map.Resize(end);

        std::fill(map.begin() + size, map.begin() + end, NativeState {
            .mapped = IL::InvalidID
        });
    }

    /// Get the current record anchor
    uint32_t GetAnchor() const {
        return allocationOffset;
    }

    /// Is an ID mapped?
    bool IsMapped(uint32_t id) const {
        return allocationOffset > id;
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
        return GetMapped(anchor - id);
    }

    /// Get a mapped value
    IL::ID GetMapped(uint64_t id) const {
        return map[id].mapped;
    }

    /// Get a mapped value
    IL::ID GetMappedCheckType(uint64_t id, DXILIDType type) const {
        ASSERT(map[id].type == type, "Unexpected type");
        return map[id].mapped;
    }

    /// Get a forward mapped value
    /// \param anchor record anchor
    /// \param id forward id
    /// \return absolute id
    IL::ID GetMappedForward(uint32_t anchor, uint32_t id) {
        NativeState& state = map[anchor + id];

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
        return map[id].type;
    }

    /// Get the internal data index of an id
    IL::ID GetDataIndex(uint64_t id) {
        return map[id].dataIndex;
    }

private:
    IL::Program& program;

private:
    struct NativeState {
        /// Program ID
        IL::ID mapped;

        /// Identifier type
        DXILIDType type;

        /// Internal data index
        uint32_t dataIndex;
    };

    /// Current allocation offset
    uint32_t allocationOffset{0};

    /// All mappings
    TrivialStackVector<NativeState, 256> map;
};
