#pragma once

// Layer
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include "DescriptorDataSegment.h"

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class DescriptorDataAppendAllocator {
public:
    DescriptorDataAppendAllocator(const ComRef<DeviceAllocator>& allocator) : allocator(allocator) {

    }

private:
    /// Device allocator
    ComRef<DeviceAllocator> allocator;
};
