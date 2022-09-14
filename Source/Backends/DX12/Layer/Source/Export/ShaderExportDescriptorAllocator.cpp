#include <Backends/DX12/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportHost.h>

ShaderExportDescriptorAllocator::ShaderExportDescriptorAllocator(ID3D12Device* device, ID3D12DescriptorHeap *heap, uint32_t bound) : heap(heap), bound(bound) {
    cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
    gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();

    descriptorAdvance = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint64_t ShaderExportDescriptorAllocator::GetDescriptorBound(ShaderExportHost *host) {
    // Entirely unsafe number of simultaneously executing command lists
    const uint32_t kMaxExecutingLists = 16384;

    // Number of exports
    uint32_t count;
    host->Enumerate(&count, nullptr);

    // Descriptor estimate
    return count * kMaxExecutingLists;
}

ShaderExportSegmentDescriptorInfo ShaderExportDescriptorAllocator::Allocate(uint32_t count) {
    // Any free'd?
    if (!freeDescriptors.empty()) {
        uint32_t id = freeDescriptors.back();
        freeDescriptors.pop_back();

        ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
        info.heap = heap;
#endif // NDEBUG
        info.offset = id;
        info.cpuHandle.ptr = cpuHandle.ptr + info.offset * descriptorAdvance;
        info.gpuHandle.ptr = gpuHandle.ptr + info.offset * descriptorAdvance;
        return info;
    }

    // Out of descriptors?
    if (slotAllocationCounter + (count - 1) >= bound) {
        return {};
    }

    // Next!
    ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
    info.heap = heap;
#endif // NDEBUG
    info.offset = slotAllocationCounter;
    info.cpuHandle.ptr = cpuHandle.ptr + info.offset * descriptorAdvance;
    info.gpuHandle.ptr = gpuHandle.ptr + info.offset * descriptorAdvance;

    slotAllocationCounter += count;

    return info;
}

void ShaderExportDescriptorAllocator::Free(const ShaderExportSegmentDescriptorInfo& id) {
#ifndef NDEBUG
    ASSERT(id.heap == heap, "Mismatched heap in shader export descriptor free");
#endif // NDEBUG
    freeDescriptors.push_back(id.offset);
}
