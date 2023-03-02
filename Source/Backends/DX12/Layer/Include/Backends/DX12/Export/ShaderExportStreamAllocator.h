#pragma once

// Common
#include <Common/IComponent.h>
#include <Common/Allocator/Vector.h>
#include <Common/Containers/ObjectPool.h>
#include <Common/Containers/TrivialObjectPool.h>
#include <Common/ComRef.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <D3D12MemAlloc.h>
#include "ShaderExportAllocationMode.h"
#include "ShaderExportStream.h"
#include "ShaderExportSegmentInfo.h"

// Std
#include <vector>

// Forward declarations
struct CommandListObject;
struct DeviceDispatchTable;
class DeviceAllocator;
struct DeviceState;

class ShaderExportStreamAllocator : public TComponent<ShaderExportStreamAllocator> {
public:
    COMPONENT(ShaderExportStreamAllocator);

    ShaderExportStreamAllocator(DeviceState *device);
    ~ShaderExportStreamAllocator();

    /// Install this allocator
    /// \return success state
    bool Install();

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
        /// Parent export id
        ShaderExportID id{0};

        /// Embedded type information
        ShaderExportTypeInfo typeInfo;

        /// Current data size
        uint64_t dataSize{0};
    };

    /// All exports
    Vector<ExportInfo> exportInfos;

private:
    ComRef<DeviceAllocator> deviceAllocator{};

    /// Pools
    ObjectPool<ShaderExportSegmentInfo> segmentPool;
    TrivialObjectPool<ShaderExportSegmentCounterInfo> counterPool;
    TrivialObjectPool<ShaderExportStreamInfo> streamPool;

    /// Initial allocation size for all streams
    uint64_t baseDataSize = 10'000;

    /// Current allocation mode
    ShaderExportAllocationMode allocationMode{ShaderExportAllocationMode::GlobalCyclicBufferNoOverwrite};
};
