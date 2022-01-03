#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/Export/DescriptorInfo.h>

// Common
#include <Common/IComponent.h>

// Std
#include <vector>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportSegmentInfo;

class ShaderExportDescriptorAllocator : public IComponent {
public:
    COMPONENT(ShaderExportDescriptorAllocator);

    ShaderExportDescriptorAllocator(DeviceDispatchTable* table);

    bool Install();

    /// Alloate a new segment
    /// \return the descriptor info
    ShaderExportSegmentDescriptorInfo Allocate();

    /// Update a segment
    /// \param info the descriptor segment to be updated
    /// \param segment the allocation segment [info] is bound to
    void Update(const ShaderExportSegmentDescriptorInfo& info, const ShaderExportSegmentInfo *segment);

    /// Free a given segment
    /// \param info
    void Free(const ShaderExportSegmentDescriptorInfo& info);

    /// Get the universal descriptor layout
    VkDescriptorSetLayout GetLayout() const {
        return layout;
    }

private:
    /// Create all dummy buffers
    void CreateDummyBuffer();

private:
    struct PoolInfo {
        uint32_t index{0};
        uint32_t freeSets{0};
        VkDescriptorPool pool{VK_NULL_HANDLE};
    };

    /// Find an existing descriptor pool or allocate a new one, which may accomodate for a new set
    /// \return the pool
    PoolInfo& FindOrAllocatePool();

    /// All pools
    std::vector<PoolInfo> pools;

private:
    /// Export record layout
    VkDescriptorSetLayout layout{nullptr};

    /// Dummy buffer
    VkBuffer dummyBuffer;
    VkBufferView dummyBufferView;
    Allocation dummyAllocation;

    /// Max sets per allocated pool
    uint32_t setsPerPool{64};

    /// The indexed bound for shader exports
    uint32_t exportBound{0};

private:
    DeviceDispatchTable* table;
};
