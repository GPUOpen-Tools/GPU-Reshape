#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
class DeviceAllocator;

struct ShaderExportConstantAllocation {
    /// Underlying resource
    ID3D12Resource* resource{nullptr};

    /// Offset into resource
    uint64_t offset{0};
};

struct ShaderExportConstantSegment {
    /// Check if this staging buffer can accommodate for a given length
    /// \param length given length
    /// \return true if this segment can accommodate
    bool CanAccomodate(size_t length) {
        return head + length <= size;
    }

    /// Underlying allocation
    Allocation allocation;

    /// Total size of this allocation
    size_t size{0};

    /// Head offset of this allocation
    size_t head{0};
};

struct ShaderExportConstantAllocator {
    /// Allocate from this constant allocator
    /// \param deviceAllocator device allocator to be used
    /// \param length length of the allocation
    /// \return given allocation
    ShaderExportConstantAllocation Allocate(const ComRef<DeviceAllocator>& deviceAllocator, size_t length);

    /// All staging buffers
    std::vector<ShaderExportConstantSegment> staging;
};
