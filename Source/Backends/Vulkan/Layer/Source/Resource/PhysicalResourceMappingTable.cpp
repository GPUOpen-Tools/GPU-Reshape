#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

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
    SegmentEntry& entry = segments.emplace_back();
    entry.id = id;
    entry.offset = head;
    entry.length = count;

    // OK
    return id;
}

void PhysicalResourceMappingTable::Free(PhysicalResourceSegmentID id) {
    uint32_t index = indices[id];

    // Not last element?
    if (index != segments.size() - 1) {
        const SegmentEntry &back = segments.back();

        // Swap move last element to current position
        indices[back.id] = index;

        // Update indices
        segments[index] = back;
    }

    segments.pop_back();

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
    virtualMappingCount = std::min(64'000u, static_cast<uint32_t>(static_cast<float>(count) * GrowthFactor));

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
            .puid = 0,
            .type = 0,
            .srb = 0
        };
    }
}

void PhysicalResourceMappingTable::Update(VkCommandBuffer commandBuffer) {
    if (!isDirty || segments.empty()) {
        return;
    }

    // Ratio threshold at which to defragment the virtual mappings
    constexpr double DefragmentationThreshold = 0.5f;

    // Determine the number of fragmented mappings
    uint64_t fragmentedMappings = SummarizeFragmentation();

    // Defragment if needed
    if (fragmentedMappings && fragmentedMappings / static_cast<double>(virtualMappingCount) >= DefragmentationThreshold) {
        Defragment();
    }

    // Number of mappings to copy
    size_t actualMappingCount = segments.back().offset + segments.back().length;

    // Copy host to device
    VkBufferCopy copyRegion;
    copyRegion.size = actualMappingCount * sizeof(VirtualResourceMapping);
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

VirtualResourceMapping* PhysicalResourceMappingTable::ModifyMappings(PhysicalResourceSegmentID id, uint32_t offset, uint32_t count) {
    isDirty = true;

    // Get the underlying segment
    SegmentEntry& segment = segments.at(indices.at(id));

    // Get mapping start
    ASSERT(offset + count <= segment.length, "Physical segment offset out of bounds");
    return virtualMappings + (segment.offset + offset);
}

void PhysicalResourceMappingTable::WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping &mapping) {
    isDirty = true;

    // Get the underlying segment
    SegmentEntry& segment = segments.at(indices.at(id));

    // Write mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    virtualMappings[segment.offset + offset] = mapping;
}

uint64_t PhysicalResourceMappingTable::SummarizeFragmentation() const {
    uint64_t fragmentedMappings{0};

    // No segments?
    if (segments.empty()) {
        return fragmentedMappings;
    }

    // Count all loose data
    for (size_t i = 0; i < segments.size() - 1; i++) {
        uint64_t end = segments[i].offset + segments[i].length;

        ASSERT(segments[i + 1].offset >= end, "Overlapping PRMT segment");
        fragmentedMappings += segments[i + 1].offset - end;
    }

    // OK
    return fragmentedMappings;
}

void PhysicalResourceMappingTable::Defragment() {
    for (size_t i = 1; i < segments.size(); i++) {
        // The optimal offset
        uint32_t relocatedOffset = segments[i - 1].offset + segments[i - 1].length;

        // Already in optimal placement?
        if (relocatedOffset == segments[i].offset) {
            continue;
        }

        // Move data to placement
        std::memmove(virtualMappings + relocatedOffset, virtualMappings + segments[i].offset, segments[i].length * sizeof(VirtualResourceMapping));

        // Update offset
        segments[i].offset = relocatedOffset;
    }
}

uint32_t PhysicalResourceMappingTable::GetSegmentShaderOffset(PhysicalResourceSegmentID id) {
    return segments.at(indices.at(id)).offset;
}

VirtualResourceMapping PhysicalResourceMappingTable::GetMapping(PhysicalResourceSegmentID id, uint32_t offset) const {
    // Get the underlying segment
    const SegmentEntry& segment = segments.at(indices.at(id));

    // Write mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    return virtualMappings[segment.offset + offset];
}
