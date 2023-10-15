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

#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Resource/VirtualResourceMapping.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTablePersistentVersion.h>

PhysicalResourceMappingTablePersistentVersion::PhysicalResourceMappingTablePersistentVersion(DeviceDispatchTable *table, const ComRef<DeviceAllocator> &allocator, uint32_t count) :
    table(table), deviceAllocator(allocator) {
    // Buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = sizeof(VirtualResourceMapping) * count;

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
}

PhysicalResourceMappingTablePersistentVersion::~PhysicalResourceMappingTablePersistentVersion() {
    // Unmap host data
    deviceAllocator->Unmap(allocation.host);

    // Destroy handles
    table->next_vkDestroyBufferView(table->object, deviceView, nullptr);
    table->next_vkDestroyBuffer(table->object, deviceBuffer, nullptr);
    table->next_vkDestroyBuffer(table->object, hostBuffer, nullptr);

    // Free underlying allocation
    deviceAllocator->Free(allocation);
}
