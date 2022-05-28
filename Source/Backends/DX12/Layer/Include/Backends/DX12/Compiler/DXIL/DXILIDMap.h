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
        IL::ID id = program.GetIdentifierMap().AllocID();

        map.Add(NativeState {
            .mapped = id,
            .type = type,
            .dataIndex = dataIndex
        });

        return id;
    }

    /// Get the current record anchor
    uint32_t GetAnchor() const {
        return map.Size();
    }

    /// Is an ID mapped?
    bool IsMapped(uint32_t id) const {
        return map.Size() > id;
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

    /// All mappings
    TrivialStackVector<NativeState, 256> map;
};
