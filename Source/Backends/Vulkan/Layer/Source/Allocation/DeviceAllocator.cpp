#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>

DeviceAllocator::~DeviceAllocator() {
    if (allocator) {
        vmaDestroyAllocator(allocator);
    }
}

void DeviceAllocator::Install(DeviceDispatchTable *table) {
    // Populate the vulkan functions
    VmaVulkanFunctions vkFunctions{};
    vkFunctions.vkAllocateMemory = table->next_vkAllocateMemory;
    vkFunctions.vkBindBufferMemory = table->next_vkBindBufferMemory;
    vkFunctions.vkBindBufferMemory2KHR = table->next_vkBindBufferMemory2KHR;
    vkFunctions.vkBindImageMemory = table->next_vkBindImageMemory;
    vkFunctions.vkBindImageMemory2KHR = table->next_vkBindImageMemory2KHR;
    vkFunctions.vkCmdCopyBuffer = table->commandBufferDispatchTable.next_vkCmdCopyBuffer;
    vkFunctions.vkCreateBuffer = table->next_vkCreateBuffer;
    vkFunctions.vkCreateImage = table->next_vkCreateImage;
    vkFunctions.vkDestroyBuffer = table->next_vkDestroyBuffer;
    vkFunctions.vkDestroyImage = table->next_vkDestroyImage;
    vkFunctions.vkFlushMappedMemoryRanges = table->next_vkFlushMappedMemoryRanges;
    vkFunctions.vkFreeMemory = table->next_vkFreeMemory;
    vkFunctions.vkGetBufferMemoryRequirements = table->next_vkGetBufferMemoryRequirements;
    vkFunctions.vkGetBufferMemoryRequirements2KHR = table->next_vkGetBufferMemoryRequirements2KHR;
    vkFunctions.vkGetImageMemoryRequirements = table->next_vkGetImageMemoryRequirements;
    vkFunctions.vkGetImageMemoryRequirements2KHR = table->next_vkGetImageMemoryRequirements2KHR;
    vkFunctions.vkGetPhysicalDeviceMemoryProperties = table->parent->next_vkGetPhysicalDeviceMemoryProperties;
    vkFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = table->parent->next_vkGetPhysicalDeviceMemoryProperties2KHR;
    vkFunctions.vkGetPhysicalDeviceProperties = table->parent->next_vkGetPhysicalDeviceProperties;
    vkFunctions.vkInvalidateMappedMemoryRanges = table->next_vkInvalidateMappedMemoryRanges;
    vkFunctions.vkMapMemory = table->next_vkMapMemory;
    vkFunctions.vkUnmapMemory = table->next_vkUnmapMemory;

    // Create the allocator
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.instance = table->parent->object;
    allocatorInfo.physicalDevice = table->physicalDevice;
    allocatorInfo.device = table->object;
    allocatorInfo.pVulkanFunctions = &vkFunctions;
    vmaCreateAllocator(&allocatorInfo, &allocator);
}

Allocation DeviceAllocator::Allocate(const VkMemoryRequirements& requirements, AllocationResidency residency) {
    Allocation allocation{};

    // Region info
    VmaAllocationCreateInfo createInfo{};

    // Translate residency
    switch (residency) {
        case AllocationResidency::Device:
            createInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            break;
        case AllocationResidency::Host:
            createInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
            break;
    }

    // Attempt to allocate the memory
    VkResult result = vmaAllocateMemory(allocator, &requirements, &createInfo, &allocation.allocation, &allocation.info);
    if (result != VK_SUCCESS) {
        return {};
    }

    return allocation;
}

MirrorAllocation DeviceAllocator::AllocateMirror(const VkMemoryRequirements& requirements, AllocationResidency residency) {
    MirrorAllocation allocation;

    switch (residency) {
        case AllocationResidency::Device:
            allocation.device = Allocate(requirements, AllocationResidency::Device);
            allocation.host = Allocate(requirements, AllocationResidency::Host);
            break;
        case AllocationResidency::Host:
            allocation.device = Allocate(requirements, AllocationResidency::Host);
            allocation.host = allocation.device;
            break;
    }

    return allocation;
}

void DeviceAllocator::Free(const Allocation& allocation) {
    vmaFreeMemory(allocator, allocation.allocation);
}

void DeviceAllocator::Free(const MirrorAllocation& mirrorAllocation) {
    if (mirrorAllocation.host.allocation != mirrorAllocation.device.allocation) {
        Free(mirrorAllocation.host);
    }

    Free(mirrorAllocation.device);
}

bool DeviceAllocator::BindBuffer(const Allocation &allocation, VkBuffer buffer) {
    return vmaBindBufferMemory(allocator, allocation.allocation, buffer) == VK_SUCCESS;
}

void *DeviceAllocator::Map(const Allocation &allocation) {
    void* data{nullptr};
    vmaMapMemory(allocator, allocation.allocation, &data);

    return data;
}

void DeviceAllocator::Unmap(const Allocation &allocation) {
    vmaUnmapMemory(allocator, allocation.allocation);
}