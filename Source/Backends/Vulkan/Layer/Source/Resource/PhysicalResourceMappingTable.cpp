#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/CommandBufferRenderPassScope.h>

PhysicalResourceMappingTable::PhysicalResourceMappingTable(DeviceDispatchTable* table) : table(table) {

}

bool PhysicalResourceMappingTable::Install() {
    deviceAllocator = registry->Get<DeviceAllocator>();

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
    uint32_t index = indices[id];

    // Last segments do not require segmentation
    if (index + 1 == segments.size()) {
        segments.pop_back();
    } else {
        // Track fragmentation
        fragmentedEntries += segments[index].length;
        
        // Let the defragmentation pick it up
        segments[index].length = 0;
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

void PhysicalResourceMappingTable::Update(CommandBufferObject* object) {
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

    // Guard against render passes
    CommandBufferRenderPassScope renderPassScope(table, object->object, &object->streamState->renderPass);

    // Copy host to device
    VkBufferCopy copyRegion;
    copyRegion.size = virtualMappingCount * sizeof(VirtualResourceMapping);
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(object->object, hostBuffer, deviceBuffer, 1u, &copyRegion);

    // Flush the copy for shader reads
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
        object->object,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    // OK
    isDirty = false;
}

VirtualResourceMapping* PhysicalResourceMappingTable::ModifyMappings(PhysicalResourceSegmentID id, uint32_t offset, uint32_t count) {
    isDirty = true;

    // Get the underlying segment
    PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Get mapping start
    ASSERT(offset + count <= segment.length, "Physical segment offset out of bounds");
    return virtualMappings + (segment.offset + offset);
}

void PhysicalResourceMappingTable::WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping &mapping) {
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
        if (segments[i].length == 0) {
            continue;
        }

        // Already in optimal placement?
        if (relocatedOffset == segments[i].offset) {
            continue;
        }

        // Move data to placement
        std::memmove(virtualMappings + relocatedOffset, virtualMappings + segments[i].offset, segments[i].length * sizeof(VirtualResourceMapping));

        // Update offset
        segments[i].offset = relocatedOffset;

        // Offset
        relocatedOffset += segments[i].length;
    }

    // Current live offset
    uint32_t liveHead = 0;

    // Remove dead segments
    for (size_t i = 0; i < segments.size(); i++) {
        const PhysicalResourceMappingTableSegment& peekSegment = segments[i];

        // Skip if removed, or not fragmented
        if (peekSegment.length == 0 || liveHead == i) {
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

PhysicalResourceMappingTableSegment PhysicalResourceMappingTable::GetSegmentShader(PhysicalResourceSegmentID id) {
    return segments.at(indices.at(id));
}

VirtualResourceMapping PhysicalResourceMappingTable::GetMapping(PhysicalResourceSegmentID id, uint32_t offset) const {
    // Get the underlying segment
    const PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Write mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    return virtualMappings[segment.offset + offset];
}
