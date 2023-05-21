#include <Backends/DX12/Export/ShaderExportConstantAllocator.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>

ShaderExportConstantAllocation ShaderExportConstantAllocator::Allocate(const ComRef<DeviceAllocator>& deviceAllocator, size_t length) {
    // Needs a staging roll?
    if (staging.empty() || !staging.back().CanAccomodate(length)) {
        // Next byte count
        const size_t lastByteCount = staging.empty() ? 16'384 : staging.back().size;
        const size_t byteCount = static_cast<size_t>(static_cast<float>(std::max<size_t>(length, lastByteCount)) * 1.5f);

        // Mapped description
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = byteCount;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleDesc.Count = 1;

        // Allocate buffer data on host, let the drivers handle page swapping
        ShaderExportConstantSegment& segment = staging.emplace_back();
        segment.allocation = deviceAllocator->Allocate(desc, AllocationResidency::Host);
        segment.size = desc.Width;

        // Map staging memory
        D3D12_RANGE range{0, 0};
        segment.allocation.resource->Map(0, &range, &segment.staging);
    }

    // Assume last staging
    ShaderExportConstantSegment& segment = staging.back();

    // Create sub-allocation
    ShaderExportConstantAllocation out;
    out.resource = segment.allocation.resource;
    out.staging = static_cast<uint8_t*>(segment.staging) + segment.head;
    out.offset = segment.head;

    // Offset head address
    segment.head += length;

    // OK
    return out;
}