// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTablePersistentVersion.h>

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
    if (persistentVersion) {
        destroyRef(persistentVersion, allocators);
    }
}

PhysicalResourceSegmentID PhysicalResourceMappingTable::Allocate(uint32_t count) {
    std::lock_guard guard(mutex);

    // Allocate block
    size_t blockOffset = partitionedAllocator.Allocate(count);

    // Out of space?
    if (blockOffset == kInvalidPartitionBlock) {
        // Allocate with safe bound
        AllocateTable(virtualMappingCount + count);

        // Re-allocate
        blockOffset = partitionedAllocator.Allocate(count);
        ASSERT(blockOffset != kInvalidPartitionBlock, "Partition re-allocation failed");
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
    entry.offset = static_cast<uint32_t>(blockOffset);
    entry.length = count;

    // Add as live
    liveSegmentCount++;

    // OK
    return id;
}

void PhysicalResourceMappingTable::Free(PhysicalResourceSegmentID id) {
    std::lock_guard guard(mutex);

    // Get segment
    uint32_t index = indices[id];
    PhysicalResourceMappingTableSegment& segment = segments[index];

    // Mark as free to the partitioned allocator
    partitionedAllocator.Free(segment.offset, segment.length);

    // Last segments do not require segmentation
    if (index + 1 == segments.size()) {
        segments.pop_back();
    } else {
        segment.destroyed = true;
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

    // Previous persistent version
    PhysicalResourceMappingTablePersistentVersion* previousVersion = persistentVersion;

    // Set new allocation count
    virtualMappingCount = std::max(64'000u, static_cast<uint32_t>(static_cast<float>(count) * GrowthFactor));

    // Set new partitioning length
    partitionedAllocator.SetLength(virtualMappingCount);

    // Create new persistent version
    persistentVersion = new (allocators) PhysicalResourceMappingTablePersistentVersion(table, deviceAllocator, virtualMappingCount);

    // Add internal user
    persistentVersion->AddUser();

    // Migrate old data
    if (previousVersion) {
        std::memcpy(persistentVersion->virtualMappings, previousVersion->virtualMappings, migratedCount * sizeof(VirtualResourceMapping));

        // Release old version
        destroyRef(previousVersion, allocators);
    }


    // Dummy initialize all new VRMs
    for (uint32_t i = migratedCount; i < virtualMappingCount; i++) {
         persistentVersion->virtualMappings[i] = VirtualResourceMapping{
            .puid = IL::kResourceTokenPUIDInvalidUndefined,
            .type = 0,
            .srb = 0
        };
    }
}

PhysicalResourceMappingTablePersistentVersion* PhysicalResourceMappingTable::GetPersistentVersion(VkCommandBuffer commandBuffer, PhysicalResourceMappingTableQueueState* queueState) {
    std::lock_guard guard(mutex);

    // Add external user
    persistentVersion->AddUser();

    // Skip updates if the queue state is at head, or if there's no segments
    if (commitHead == queueState->commitHead || !liveSegmentCount) {
        return persistentVersion;
    }

    // Copy host to device
    VkBufferCopy copyRegion;
    copyRegion.size = virtualMappingCount * sizeof(VirtualResourceMapping);
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(commandBuffer, persistentVersion->hostBuffer, persistentVersion->deviceBuffer, 1u, &copyRegion);

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

    // Set new head
    queueState->commitHead = commitHead;

    // OK
    return persistentVersion;
}

void PhysicalResourceMappingTable::WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping &mapping) {
    std::lock_guard guard(mutex);

    // Get the underlying segment
    PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Write mapping
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    persistentVersion->virtualMappings[segment.offset + offset] = mapping;

    // Advance head
    commitHead++;
}

size_t PhysicalResourceMappingTable::GetMappingOffset(PhysicalResourceSegmentID id, uint32_t offset) {
    std::lock_guard guard(mutex);
    
    // Get the underlying segment
    const PhysicalResourceMappingTableSegment& segment = segments.at(indices.at(id));

    // Get mapping offset
    ASSERT(offset < segment.length, "Physical segment offset out of bounds");
    return segment.offset + offset;
}

void PhysicalResourceMappingTable::CopyMappings(PhysicalResourceSegmentID source, PhysicalResourceSegmentID dest) {
    std::lock_guard guard(mutex);
    
    // Get the underlying segment
    const PhysicalResourceMappingTableSegment& sourceSegment = segments.at(indices.at(source));
    const PhysicalResourceMappingTableSegment& destSegment   = segments.at(indices.at(dest));

    // Validation
    ASSERT(sourceSegment.length == destSegment.length, "Length mismatch");

    // Copy range
    std::memcpy(
        persistentVersion->virtualMappings + sourceSegment.offset,
        persistentVersion->virtualMappings + destSegment.offset,
        sourceSegment.length
    );
    
    // Advance head
    commitHead++;
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
    return persistentVersion->virtualMappings[segment.offset + offset];
}
