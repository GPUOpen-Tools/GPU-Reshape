// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/CommandBufferRenderPassScope.h>

PhysicalResourceMappingTable::PhysicalResourceMappingTable(DeviceDispatchTable* table) : table(table) {

}

bool PhysicalResourceMappingTable::Install() {
    deviceAllocator = registry->Get<DeviceAllocator>();

    // Dummy allocation for fast nullability
    //  This effectively invalidates the PRMT at 0 / null, which allows for fast shader checking without
    //  needing additional guarding on the subsequent loads.
    Allocate(4u);

    // OK
    return true;
}

PhysicalResourceMappingTable::~PhysicalResourceMappingTable() {
    if (!virtualMappings) {
        return;
    }

    // Unmap host data
    deviceAllocator->Unmap(allocation.host);

    // Destroy handles
    table->next_vkDestroyBufferView(table->object, deviceView, nullptr);
    table->next_vkDestroyBuffer(table->object, deviceBuffer, nullptr);
    table->next_vkDestroyBuffer(table->object, hostBuffer, nullptr);

    // Free underlying allocation
    deviceAllocator->Free(allocation);
}

uint32_t PhysicalResourceMappingTable::GetHeadOffset() const {
    if (segments.empty()) {
        return 0;
    }

    return segments.back().offset + segments.back().length;
}

PhysicalResourceSegmentID PhysicalResourceMappingTable::Allocate(uint32_t count) {
    std::lock_guard guard(mutex);
    
    uint32_t head = GetHeadOffset();

    // Out of (potentially fragmented) space?
    if (head + count >= virtualMappingCount) {
        // TODO: We could try to defragment here, but I don't think that's worth it
        //       It could lead to constant micro-defragmentation that is better handled later.
        AllocateTable(head + count);
    }

    // Determine index
    PhysicalResourceSegmentID id;
    if (freeIndices.empty()) {
        // Allocate at end
        id = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        id = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[id] = static_cast<uint32_t>(segments.size());

    // Create new segment
    PhysicalResourceMappingTableSegment& entry = segments.emplace_back();
    entry.id = id;
    entry.offset = head;
    entry.length = count;

    // Add as live
    liveSegmentCount++;

    // OK
    return id;
}

void PhysicalResourceMappingTable::Free(PhysicalResourceSegmentID id) {
    std::lock_guard guard(mutex);
    
    uint32_t index = indices[id];

    // Last segments do not require segmentation
    if (index + 1 == segments.size()) {
        segments.pop_back();
    } else {
        // Track fragmentation
        fragmentedEntries += segments[index].length;
        
        // Let the defragmentation pick it up
        segments[index].destroyed = true;
    }

    // Not live
    liveSegmentCount--;
    
    // Add as free index
    freeIndices.push_back(id);
}

void PhysicalResourceMappingTable::AllocateTable(uint32_t count) {
    constexpr float GrowthFactor = 1.5f;

    // Number of old mappings
    uint32_t migratedCount = virtualMappingCount;

    // Old mapping data
    const VirtualResourceMapping* migratedMappings = virtualMappings;

    // Set new allocation count
    virtualMappingCount = std::max(64'000u, static_cast<uint32_t>(static_cast<float>(count) * GrowthFactor));

    // Buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = sizeof(VirtualResourceMapping) * virtualMappingCount;

    // Attempt to create the buffers
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &hostBuffer) != VK_SUCCESS ||
        table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &deviceBuffer) != VK_SUCCESS) {
        return;
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, deviceBuffer, &requirements);

    // Create the allocation
    allocation = deviceAllocator->AllocateMirror(requirements);

    // Bind against the allocation
    deviceAllocator->BindBuffer(allocation.host, hostBuffer);
    deviceAllocator->BindBuffer(allocation.device, deviceBuffer);

    // View creation info
    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    viewInfo.buffer = deviceBuffer;
    viewInfo.format = VK_FORMAT_R32_UINT;
    viewInfo.range = VK_WHOLE_SIZE;

    // Create the view
    if (table->next_vkCreateBufferView(table->object, &viewInfo, nullptr, &deviceView) != VK_SUCCESS) {
        return;
    }

    // Map the host data (persistent)
    virtualMappings = static_cast<VirtualResourceMapping*>(deviceAllocator->Map(allocation.host));

    // Migrate old data
    std::memcpy(virtualMappings, migratedMappings, migratedCount * sizeof(VirtualResourceMapping));

    // Dummy initialize all new VRMs
    for (uint32_t i = migratedCount; i < virtualMappingCount; i++) {
        virtualMappings[i] = VirtualResourceMapping{
            .puid = IL::kResourceTokenPUIDInvalidUndefined,
            .type = 0,
            .srb = 0
        };
    }
}

void PhysicalResourceMappingTable::Update(VkCommandBuffer commandBuffer) {
    std::lock_guard guard(mutex);
    
    if (!isDirty || !liveSegmentCount) {
        return;
    }

    // Ratio threshold at which to defragment the virtual mappings
    constexpr double DefragmentationThreshold = 0.5f;

    // Validate ranges
#if !defined(NDEBUG)
    // Count all loose data
    for (size_t i = 0; i < segments.size() - 1; i++) {
        uint64_t end = segments[i].offset + segments[i].length;
        ASSERT(segments[i + 1].offset >= end, "Overlapping PRMT segment");
    }
#endif // !defined(NDEBUG)

    // Defragment if needed
    if (fragmentedEntries && fragmentedEntries / static_cast<double>(virtualMappingCount) >= DefragmentationThreshold) {
        Defragment();
    }

    // Copy host to device
    VkBufferCopy copyRegion;
    copyRegion.size = virtualMappingCount * sizeof(VirtualResourceMapping);
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(commandBuffer, hostBuffer, deviceBuffer, 1u, &copyRegion);

    // Flush the copy for shader reads
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    // OK
    isDirty = false;
}

void PhysicalResourceMappingTable::WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping &mapping) {
    std::lock_guard guard(mutex);
    isDirty = true;

    // Get the underlying segment
    PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Write mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    virtualMappings[segment.offset + offset] = mapping;
}

void PhysicalResourceMappingTable::Defragment() {
    // The current optimal offset
    uint32_t relocatedOffset = 0;

    // Defragment all live mappings
    for (size_t i = 0; i < segments.size(); i++) {
        // Removed segment?
        if (segments[i].destroyed) {
            continue;
        }

        // Already in optimal placement?
        if (relocatedOffset != segments[i].offset) {
            // Move data to placement
            std::memmove(virtualMappings + relocatedOffset, virtualMappings + segments[i].offset, segments[i].length * sizeof(VirtualResourceMapping));

            // Update offset
            segments[i].offset = relocatedOffset;
        }

        // Offset
        relocatedOffset += segments[i].length;
    }

    // Current live offset
    uint32_t liveHead = 0;

    // Remove dead segments
    for (size_t i = 0; i < segments.size(); i++) {
        const PhysicalResourceMappingTableSegment& peekSegment = segments[i];

        // Skip if removed
        if (peekSegment.destroyed) {
            continue;
        }

        // Move peek segment to current
        segments[liveHead] = peekSegment;

        // Update lookup for peek segment to live
        indices[peekSegment.id] = liveHead;

        // Next!
        liveHead++;
    }

    // Remove dead segments
    segments.erase(segments.begin() + liveHead, segments.end());

    // Validate
    ASSERT(liveHead == liveSegmentCount, "Invalid tracked live count to segment");

    // No longer fragmented
    fragmentedEntries = 0;
}

size_t PhysicalResourceMappingTable::GetMappingOffset(PhysicalResourceSegmentID id, uint32_t offset) {
    std::lock_guard guard(mutex);
    
    // Get the underlying segment
    const PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Get mapping offset
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    return segment.offset + offset;
}

PhysicalResourceMappingTableSegment PhysicalResourceMappingTable::GetSegmentShader(PhysicalResourceSegmentID id) {
    std::lock_guard guard(mutex);
    return segments.at(indices.at(id));
}

VirtualResourceMapping PhysicalResourceMappingTable::GetMapping(PhysicalResourceSegmentID id, uint32_t offset) {
    std::lock_guard guard(mutex);
    
    // Get the underlying segment
    const PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Get mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    return virtualMappings[segment.offset + offset];
}
