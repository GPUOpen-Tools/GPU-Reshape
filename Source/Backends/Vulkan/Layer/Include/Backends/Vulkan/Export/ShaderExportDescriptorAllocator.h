#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/Export/DescriptorInfo.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <Backends/Vulkan/States/PipelineLayoutBindingInfo.h>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportSegmentInfo;
class DeviceAllocator;

class ShaderExportDescriptorAllocator : public TComponent<ShaderExportDescriptorAllocator> {
public:
    COMPONENT(ShaderExportDescriptorAllocator);

    ShaderExportDescriptorAllocator(DeviceDispatchTable* table);

    ~ShaderExportDescriptorAllocator();

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

    /// Get the shared binding info
    PipelineLayoutBindingInfo GetBindingInfo() const {
        return bindingInfo;
    }

private:
    /// Create all dummy buffers
    void CreateDummyBuffer();

    /// Create the shared binding info
    void CreateBindingLayout();

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

    /// Shared binding info
    PipelineLayoutBindingInfo bindingInfo;

    /// Dummy buffer
    VkBuffer dummyBuffer;
    VkBufferView dummyBufferView;
    Allocation dummyAllocation;

    /// Max sets per allocated pool
    uint32_t setsPerPool{64};

    /// The indexed bound for shader exports
    uint32_t exportBound{0};

    /// The indexed bound for data resources
    std::vector<ShaderDataInfo> dataResources;

private:
    DeviceDispatchTable* table;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator;
};
