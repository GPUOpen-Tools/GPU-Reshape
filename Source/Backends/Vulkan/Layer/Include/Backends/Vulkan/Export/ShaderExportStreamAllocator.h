#pragma once

// Common
#include <Common/IComponent.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/Vulkan/VMA.h>
#include <Backends/Vulkan/ReferenceObject.h>
#include "ShaderExportAllocationMode.h"
#include "ShaderExportStream.h"

// Std
#include <vector>

// Forward declarations
struct CommandBufferObject;
struct DeviceDispatchTable;

class ShaderExportStreamAllocator : public IComponent {
public:
    COMPONENT(ShaderExportStreamAllocator);

    ShaderExportStreamAllocator(DeviceDispatchTable* table);

    void SetAllocationMode(ShaderExportAllocationMode mode);

    void SetCounter(ShaderExportID id, uint64_t counter);

    ShaderExportStream Allocate(ShaderExportID id, const ShaderExportTypeInfo& typeInfo);

private:
    /// A single segment allocation
    struct StreamSegment : public ReferenceObject {
        VkBuffer buffer;
        VmaAllocation dataAllocation{nullptr};
    };

    struct CounterInfo {
        VkBuffer buffer;
        VmaAllocation allocation;
        CommandBufferObject
    };

    struct StreamInfo {
        ShaderExportTypeInfo typeInfo;
        ShaderExportStream stream;

        uint64_t size{0};
        uint64_t counter{0};
    };

    StreamInfo& GetStreamInfo(ShaderExportID id);

    void CreateStream(StreamInfo& info);

private:
    std::vector<CounterInfo> counters;

    std::vector<uint32_t> liveCounters;
    std::vector<uint32_t> freeCounters;

    std::vector<StreamInfo> streams;

    uint64_t baseDataSize = 10'000;

    float growthFactor = 1.5f;

    ShaderExportAllocationMode allocationMode{ShaderExportAllocationMode::GlobalCyclicBufferNoOverwrite};

private:
    DeviceDispatchTable* table;
};
