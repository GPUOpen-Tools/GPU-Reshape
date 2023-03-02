#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Std
#include <vector>

struct DescriptorDataSegmentEntry {
    /// Allocation of this segment entry
    Allocation allocation;
};

struct DescriptorDataSegment {
    DescriptorDataSegment(const Allocators& allocators) : entries(allocators) {
        
    }
    
    /// All entries within this segment
    Vector<DescriptorDataSegmentEntry> entries;
};
