#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Allocation/Allocation.h>

// Common
#include <Common/IComponent.h>

// Std
#include <vector>
#include "DescriptorInfo.h"

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportSegmentInfo;

class ShaderExportDescriptorAllocator : public IComponent {
public:
    COMPONENT(ShaderExportDescriptorAllocator);

    ShaderExportDescriptorAllocator(DeviceDispatchTable* table);

    void Install();

    ShaderExportSegmentDescriptorInfo Allocate();

    void Update(const ShaderExportSegmentDescriptorInfo& info, const ShaderExportSegmentInfo *segment);

    void Free(const ShaderExportSegmentDescriptorInfo& info);

    VkDescriptorSetLayout GetLayout() const {
        return layout;
    }

private:
    void CreateDummyBuffer();

private:
    struct PoolInfo {
        uint32_t index{0};
        uint32_t freeSets{0};
        VkDescriptorPool pool{VK_NULL_HANDLE};
    };

    PoolInfo& FindOrAllocatePool();

    std::vector<PoolInfo> pools;

private:
    VkDescriptorSetLayout layout{nullptr};

    VkBuffer dummyBuffer;
    VkBufferView dummyBufferView;
    Allocation dummyAllocation;

    uint32_t setsPerPool{64};

    uint32_t exportBound{0};

private:
    DeviceDispatchTable* table;
};
