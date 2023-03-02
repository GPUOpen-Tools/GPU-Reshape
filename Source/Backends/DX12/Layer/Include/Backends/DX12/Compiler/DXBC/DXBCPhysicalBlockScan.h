#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include "DXBCPhysicalBlock.h"
#include "DXBCPhysicalBlockType.h"
#include "DXBCHeader.h"

// Common
#include <Common/Allocator/Vector.h>
#include <Common/Allocators.h>

// Forward declarations
struct DXStream;
struct DXJob;

/// DXBC Scanner
struct DXBCPhysicalBlockScan {
public:
    DXBCPhysicalBlockScan(const Allocators& allocators);

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Scan(const void* byteCode, uint64_t byteLength);

    /// Stitch the resulting stream
    /// \param out
    void Stitch(const DXJob& job, DXStream& out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXBCPhysicalBlockScan& out);

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
        Section(const Allocators& allocators) : block(allocators) {
            
        }
    
        uint32_t unexposedType;
        DXBCPhysicalBlockType type;
        DXBCPhysicalBlock block;
    };

    /// All sections
    Vector<Section> sections;

private:
    Allocators allocators;
};
