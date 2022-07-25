#pragma once

// Layer
#include <Backend/IL/ID.h>

// Std
#include <vector>

struct DXILIDRemapper {
    /// Set the remap bound
    /// \param source source program bound
    /// \param user user modified program bound
    void SetBound(uint32_t source, uint32_t user) {
        sourceMappings.resize(source, ~0u);
        userMappings.resize(user, ~0u);
    }

    /// Allocate a source record value
    void AllocSourceMapping() {
        sourceMappings[sourceAllocationIndex++] = allocationIndex++;
    }

    /// Remap a DXIL value
    /// \param source source DXIL value
    /// \return remapped value
    uint32_t Remap(uint32_t source) {
        return sourceMappings.at(source);
    }

    /// Try to remap a DXIL value
    /// \param source source DXIL value
    /// \return remapped value, UINT32_MAX if not found
    uint32_t TryRemap(uint32_t source) {
        return source < sourceMappings.size() ? sourceMappings.at(source) : ~0u;
    }

    /// Allocate a user mapping
    /// \param id the IL id
    void AllocUserMapping(IL::ID id) {
        userMappings.at(id) = allocationIndex++;
    }

private:
    /// Current user allocation index
    uint32_t allocationIndex{0};

    /// Current source allocation index
    uint32_t sourceAllocationIndex{0};

    /// All present source mappings
    std::vector<uint32_t> sourceMappings;

    /// All present user mappings
    std::vector<uint32_t> userMappings;
};
