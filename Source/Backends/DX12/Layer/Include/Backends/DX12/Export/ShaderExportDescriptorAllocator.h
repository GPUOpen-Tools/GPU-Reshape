#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>

// Std
#include <vector>

// Forward declarations
class ShaderExportHost;

class ShaderExportDescriptorAllocator {
public:
    /// Constructor
    /// \param device parent device
    /// \param heap target heap
    /// \param bound expected bound
    ShaderExportDescriptorAllocator(ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t bound);

    /// Allocate a new descriptor
    /// \param count number of descriptors to allocate
    /// \return descriptor base info
    ShaderExportSegmentDescriptorInfo Allocate(uint32_t count);

    /// Free a descriptor
    /// \param id
    void Free(const ShaderExportSegmentDescriptorInfo& id);

    /// Get the allocation prefix
    uint32_t GetPrefix() const {
        return bound;
    }

    /// Get the allocation prefix offset
    uint64_t GetPrefixOffset() const {
        return bound * descriptorAdvance;
    }

    /// Get the descriptor ptr advance
    uint32_t GetAdvance() const {
        return descriptorAdvance;
    }

public:
    /// Get a safe bound
    /// \param host shader export host
    /// \return descriptor count bound
    static uint32_t GetDescriptorBound(ShaderExportHost* host);

private:
    /// Current allocation counter
    uint32_t slotAllocationCounter{0};

    /// Currently free descriptors
    std::vector<uint32_t> freeDescriptors;

    /// Parent bound
    uint32_t bound;

    /// Parent heap
    ID3D12DescriptorHeap* heap;

    /// Heap advance
    uint32_t descriptorAdvance;

    /// Base CPU handle
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;

    /// Base GPU handle
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

