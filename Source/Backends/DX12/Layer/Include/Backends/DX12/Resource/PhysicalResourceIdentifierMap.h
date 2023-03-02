#pragma once

// Layer
#include <Backend/IL/ResourceTokenPacking.h>

// Common
#include <Common/Assert.h>

// Std
#include <vector>

struct PhysicalResourceIdentifierMap {
    PhysicalResourceIdentifierMap(const Allocators& allocators) : freePUIDs(allocators) {
        
    }
    
    /// Allocate a new PUID
    /// \return
    uint32_t AllocatePUID() {
        if (freePUIDs.empty()) {
            ASSERT(puidHead < IL::kResourceTokenPUIDMask, "Exceeded maximum resource count");
            return puidHead++;
        }

        uint32_t puid = freePUIDs.back();
        freePUIDs.pop_back();
        return puid;
    }

    /// Free a puid
    /// \param puid
    void FreePUID(uint32_t puid) {
        freePUIDs.push_back(puid);
    }

private:
    /// Current head counter
    uint32_t puidHead{0};

    /// All free indices
    Vector<uint32_t> freePUIDs;
};
