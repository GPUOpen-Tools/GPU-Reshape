#pragma once

// Layer
#include "Spv.h"

// Std
#include <vector>

struct SpvIdMap {
    /// Set the identifier bound
    /// \param value address of bound
    void SetBound(uint32_t* value) {
        allocatedBound = value;

        idLookup.resize(*value);
        for (uint32_t i = 0; i < *value; i++) {
            idLookup[i] = i;
        }
    }

    /// Allocate a new identifier
    /// \return
    SpvId Allocate() {
        SpvId id = (*allocatedBound)++;
        idLookup.emplace_back(id);
        return id;
    }

    /// Get an identifier
    SpvId Get(SpvId id) {
        return idLookup[id];
    }

    /// Set a relocation identifier
    /// \param id the relocation identifier
    /// \param value the value assigned
    void Set(SpvId id, SpvId value) {
        idLookup.at(id) = value;
    }

    /// Get the current bound
    uint32_t GetBound() const  {
        return *allocatedBound;
    }

private:
    /// Bound address
    uint32_t* allocatedBound{nullptr};

    /// All relocations
    std::vector<SpvId> idLookup;
};
