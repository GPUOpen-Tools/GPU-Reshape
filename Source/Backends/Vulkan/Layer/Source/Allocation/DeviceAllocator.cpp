// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>

// Backend
#include <Backend/Diagnostic/DiagnosticFatal.h>

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
        case AllocationResidency::HostVisible:
            createInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;
            createInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            createInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
    }

    // Attempt to allocate the memory
    VkResult result = vmaAllocateMemory(allocator, &requirements, &createInfo, &allocation.allocation, &allocation.info);
    if (result != VK_SUCCESS) {
        // Display friendly message
        Backend::DiagnosticFatal(
            "Out Of Memory",
            "GPU Reshape has run out of {} memory. Please consider decreasing the workload "
            "or simplifying instrumentation (e.g., disabling texel addressing)",
            residency == AllocationResidency::Device ? "device-local" : "system"
        );

        // Unreachable
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
        case AllocationResidency::HostVisible:
            allocation.device = Allocate(requirements, AllocationResidency::HostVisible);
            allocation.host = allocation.device;
            break;
    }

    return allocation;
}

VmaAllocation DeviceAllocator::AllocateMemory(const VkMemoryRequirements& requirements) {
    // Default to GPU memory
    VmaAllocationCreateInfo createInfo{};
    createInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // Try to create allocation
    VmaAllocation allocation;
    if (vmaAllocateMemory(
        allocator,
        &requirements,
        &createInfo,
        &allocation,
        nullptr
    ) != VK_SUCCESS) {
        // Display friendly message
        Backend::DiagnosticFatal(
            "Out Of Memory",
            "GPU Reshape has run out of virtual backing memory. Please consider decreasing the workload "
            "or disabling texel addressing."
        );

        // Unreachable
        return nullptr;
    }

    // OK
    return allocation;
}

VmaAllocationInfo DeviceAllocator::GetAllocationInfo(VmaAllocation allocation) {
    VmaAllocationInfo info{};
    vmaGetAllocationInfo(allocator, allocation, &info);
    return info;
}

void DeviceAllocator::Free(VmaAllocation allocation) {
    vmaFreeMemory(allocator, allocation);
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
    if (allocation.info.pMappedData) {
        return allocation.info.pMappedData;
    }

    void* data{nullptr};
    vmaMapMemory(allocator, allocation.allocation, &data);

    return data;
}

void DeviceAllocator::Unmap(const Allocation &allocation) {
    vmaUnmapMemory(allocator, allocation.allocation);
}

void DeviceAllocator::FlushMappedRange(const Allocation &allocation, uint64_t offset, uint64_t length) {
    vmaFlushAllocation(allocator, allocation.allocation, offset, length);
}
