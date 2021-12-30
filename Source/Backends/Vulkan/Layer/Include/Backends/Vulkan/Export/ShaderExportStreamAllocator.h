#pragma once

// Common
#include <Common/IComponent.h>
#include <Common/ObjectPool.h>
#include <Common/TrivialObjectPool.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/Vulkan/VMA.h>
#include "ShaderExportAllocationMode.h"
#include "ShaderExportStream.h"
#include "SegmentInfo.h"

// Std
#include <vector>

// Forward declarations
struct CommandBufferObject;
struct DeviceDispatchTable;
class DeviceAllocator;

class ShaderExportStreamAllocator : public IComponent {
public:
    COMPONENT(ShaderExportStreamAllocator);

    ShaderExportStreamAllocator(DeviceDispatchTable* table);

    void Install();

    /// Allocate a new allocation
    /// \return the allocation
    ShaderExportSegmentInfo* AllocateSegment();

    /// Free an existing allocation
    /// \param segment allocation to be free'd
    void FreeSegment(ShaderExportSegmentInfo* segment);

    /// Set the size of a shader export stream
    ///   ! Incurs potential segmentation on the next allocation
    /// \param id the shader export id
    /// \param size the byte size of the new stream
    void SetStreamSize(ShaderExportID id, uint64_t size);

private:
    /// Allocate a new stream
    /// \param id the export id
    /// \return stream info
    ShaderExportStreamInfo AllocateStreamInfo(const ShaderExportID& id);

    /// Allocate a new counter
    /// \return counter info
    ShaderExportSegmentCounterInfo AllocateCounterInfo();

private:
    struct ExportInfo {
        ShaderExportID id{0};
        ShaderExportTypeInfo typeInfo;
        uint64_t dataSize{0};
    };

    std::vector<ExportInfo> exportInfos;

private:
    DeviceAllocator* deviceAllocator{};

    /// Pools
    ObjectPool<ShaderExportSegmentInfo> segmentPool;
    TrivialObjectPool<ShaderExportSegmentCounterInfo> counterPool;
    TrivialObjectPool<ShaderExportStreamInfo> streamPool;

    /// Initial allocation size for all streams
    uint64_t baseDataSize = 10'000;

    ShaderExportAllocationMode allocationMode{ShaderExportAllocationMode::GlobalCyclicBufferNoOverwrite};

private:
    DeviceDispatchTable* table;
};
