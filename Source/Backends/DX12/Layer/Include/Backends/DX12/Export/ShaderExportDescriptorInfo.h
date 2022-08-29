#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Std
#include <cstdint>

/// A single allocation descriptor allocation
struct ShaderExportSegmentDescriptorInfo {
    /// Originating heap
    ID3D12DescriptorHeap* heap{};

    /// Base CPU handle
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;

    /// Base GPU handle
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;

    /// Offset within the heap
    uint32_t offset{};
};
