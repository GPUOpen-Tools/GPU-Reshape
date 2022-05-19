#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include "DXBCPhysicalBlock.h"
#include "DXBCPhysicalBlockType.h"
#include "DXBCHeader.h"

// Common
#include <Common/Allocators.h>

// Std
#include <vector>

/// DXBC Scanner
struct DXBCPhysicalBlockScan {
public:
    DXBCPhysicalBlockScan(const Allocators& allocators);

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Scan(const void* byteCode, uint64_t byteLength);

    /// Get a physical block of type
    /// \return nullptr if not found
    DXBCPhysicalBlock* GetPhysicalBlock(DXBCPhysicalBlockType type) {
        for (Section& section : sections) {
            if (section.type == type) {
                return &section.block;
            }
        }

        // Not found
        return nullptr;
    }

    /// Top header
    DXBCHeader header;

private:
    struct Section {
        DXBCPhysicalBlockType type;
        DXBCPhysicalBlock block;
    };

    /// All sections
    std::vector<Section> sections;

private:
    Allocators allocators;
};
