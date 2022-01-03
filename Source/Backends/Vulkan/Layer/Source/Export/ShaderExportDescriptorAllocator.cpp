#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Export/SegmentInfo.h>

// Common
#include <Common/Registry.h>

// Backend
#include <Backend/IShaderExportHost.h>

ShaderExportDescriptorAllocator::ShaderExportDescriptorAllocator(DeviceDispatchTable *table) : table(table) {
}

bool ShaderExportDescriptorAllocator::Install() {
    auto *host = registry->Get<IShaderExportHost>();
    host->Enumerate(&exportBound, nullptr);

    // Binding for counter data
    VkDescriptorSetLayoutBinding counterLayout{};
    counterLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterLayout.stageFlags = VK_SHADER_STAGE_ALL;
    counterLayout.descriptorCount = 1;
    counterLayout.binding = 0;

    // Binding for stream data
    VkDescriptorSetLayoutBinding streamLayout{};
    streamLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    streamLayout.stageFlags = VK_SHADER_STAGE_ALL;
    streamLayout.descriptorCount = exportBound;
    streamLayout.binding = 1;

    // All bindings
    VkDescriptorSetLayoutBinding bindings[] = {counterLayout, streamLayout};

    // Binding flags
    //  ? Descriptors are updated latent, during recording we do not know what segment
    //    is appropriate until submission.
    VkDescriptorBindingFlags bindingFlags[] = { VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };

    // Binding create info
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    bindingCreateInfo.bindingCount = 2;
    bindingCreateInfo.pBindingFlags = bindingFlags;

    // Set layout create info
    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.pNext = &bindingCreateInfo;
    setInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;;
    setInfo.bindingCount = 2; /* Counters + Streams */
    setInfo.pBindings = bindings;
    if (table->next_vkCreateDescriptorSetLayout(table->object, &setInfo, nullptr, &layout) != VK_SUCCESS) {
        return false;
    }

    // Create the dummy buffer
    CreateDummyBuffer();

    return true;
}

ShaderExportSegmentDescriptorInfo ShaderExportDescriptorAllocator::Allocate() {
    PoolInfo &pool = FindOrAllocatePool();

    // Decrement available sets
    pool.freeSets--;
    ASSERT(pool.freeSets < setsPerPool, "Invalid pool state, max sets per pool exceeded (underflow)");

    // Allocation info
    VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool = pool.pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layout;

    // Setup the descriptor info
    ShaderExportSegmentDescriptorInfo info;
    info.poolIndex = pool.index;

    // Attempt to allocate a set
    if (table->next_vkAllocateDescriptorSets(table->object, &allocateInfo, &info.set) != VK_SUCCESS) {
        return {};
    }

    // Single counter
    VkWriteDescriptorSet counterWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    counterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterWrite.descriptorCount = 1u;
    counterWrite.pTexelBufferView = &dummyBufferView;
    counterWrite.dstArrayElement = 0;
    counterWrite.dstSet = info.set;
    counterWrite.dstBinding = 0;

    VkWriteDescriptorSet streamWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    streamWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    streamWrite.descriptorCount = 1u;
    streamWrite.pTexelBufferView = &dummyBufferView;
    streamWrite.dstArrayElement = 0;
    streamWrite.dstSet = info.set;
    streamWrite.dstBinding = 1u;

    // Combined writes
    VkWriteDescriptorSet writes[] = {
    counterWrite,
    streamWrite
    };

    // Update the descriptor set
    table->next_vkUpdateDescriptorSets(table->object, 2u, writes, 0, nullptr);

    // OK
    return info;
}

ShaderExportDescriptorAllocator::PoolInfo &ShaderExportDescriptorAllocator::FindOrAllocatePool() {
    // Check existing pools
    for (auto it = pools.rbegin(); it != pools.rend(); it++) {
        if (it->freeSets > 0) {
            return *it;
        }
    }

    // Pool size, both the counter and stream are uniform texel buffers, one of each (hence x2)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    poolSize.descriptorCount = exportBound * 2;

    // Descriptor pool create info
    VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    createInfo.maxSets = setsPerPool;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;

    // Create new info
    PoolInfo &info = pools.emplace_back();
    info.index = pools.size() - 1;
    info.freeSets = setsPerPool;

    // Attempt to create the pool
    if (table->next_vkCreateDescriptorPool(table->object, &createInfo, nullptr, &info.pool) != VK_SUCCESS) {
        return info;
    }

    // OK
    return info;
}

void ShaderExportDescriptorAllocator::Free(const ShaderExportSegmentDescriptorInfo &info) {
    PoolInfo &pool = pools[info.poolIndex];

    // Increment set count
    pool.freeSets++;
    ASSERT(pool.freeSets <= setsPerPool, "Invalid pool state, max sets per pool exceeded");

    // Free the set
    if (table->next_vkFreeDescriptorSets(table->object, pool.pool, 1, &info.set) != VK_SUCCESS) {
        // ...
    }
}

void ShaderExportDescriptorAllocator::Update(const ShaderExportSegmentDescriptorInfo &info, const ShaderExportSegmentInfo *segment) {
    auto *streamWrites = ALLOCA_ARRAY(VkBufferView, segment->streams.size());

    // Copy views
    for (size_t i = 0; i < segment->streams.size(); i++) {
        streamWrites[i] = segment->streams[i].view;
    }

    // Single counter
    VkWriteDescriptorSet counterWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    counterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterWrite.descriptorCount = 1u;
    counterWrite.pTexelBufferView = &segment->counter.view;
    counterWrite.dstArrayElement = 0;
    counterWrite.dstSet = info.set;
    counterWrite.dstBinding = 0;

    // Skip stream writing if empty
    if (segment->streams.empty()) {
        // Update the descriptor set
        table->next_vkUpdateDescriptorSets(table->object, 1u, &counterWrite, 0, nullptr);
    } else {
        // All streams
        VkWriteDescriptorSet streamWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        streamWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        streamWrite.descriptorCount = segment->streams.size();
        streamWrite.pTexelBufferView = streamWrites;
        streamWrite.dstArrayElement = 0;
        streamWrite.dstSet = info.set;
        streamWrite.dstBinding = 1;

        // Combined writes
        VkWriteDescriptorSet writes[] = {
        counterWrite,
        streamWrite
        };

        // Update the descriptor set
        table->next_vkUpdateDescriptorSets(table->object, 2u, writes, 0, nullptr);
    }
}

void ShaderExportDescriptorAllocator::CreateDummyBuffer() {
    auto deviceAllocator = registry->Get<DeviceAllocator>();

    // Dummy buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = sizeof(ShaderExportCounter);

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &dummyBuffer) != VK_SUCCESS) {
        return;
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, dummyBuffer, &requirements);

    // Create the allocation
    dummyAllocation = deviceAllocator->Allocate(requirements);

    // Bind against the device allocation
    deviceAllocator->BindBuffer(dummyAllocation, dummyBuffer);

    // View creation info
    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    viewInfo.buffer = dummyBuffer;
    viewInfo.format = VK_FORMAT_R32_UINT;
    viewInfo.range = VK_WHOLE_SIZE;

    // Create the view
    if (table->next_vkCreateBufferView(table->object, &viewInfo, nullptr, &dummyBufferView) != VK_SUCCESS) {
        return;
    }
}
