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

#include <Backends/Vulkan/Export/ShaderExportConstantAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>

ShaderExportConstantAllocation ShaderExportConstantAllocator::Allocate(DeviceDispatchTable* table, size_t length, size_t align) {
    // Needs a staging roll?
    if (staging.empty() || !staging.back().CanAccomodate(length, align)) {
        // Next byte count
        const size_t lastByteCount = staging.empty() ? 16'384 : staging.back().size;
        const size_t byteCount = static_cast<size_t>(static_cast<float>(std::max<size_t>(length, lastByteCount)) * 1.5f);

        ShaderExportConstantSegment& segment = staging.emplace_back();
        segment.size = byteCount;
        
        // Mapped description
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        info.size = byteCount;

        // Attempt to create the host buffer
        if (table->next_vkCreateBuffer(table->object, &info, nullptr, &segment.buffer) != VK_SUCCESS) {
            return {};
        }

        // Get the requirements
        VkMemoryRequirements requirements;
        table->next_vkGetBufferMemoryRequirements(table->object, segment.buffer, &requirements);
        
        // Allocate buffer data on host, let the drivers handle page swapping
        segment.allocation = table->deviceAllocator->Allocate(requirements, AllocationResidency::Host);
        segment.staging = table->deviceAllocator->Map(segment.allocation);
        
        // Bind against the allocations
        table->deviceAllocator->BindBuffer(segment.allocation, segment.buffer);
    }

    // Assume last staging
    ShaderExportConstantSegment& segment = staging.back();

    // Align to expectations
    segment.head = (segment.head + align - 1) & ~(align - 1);

    // Create sub-allocation
    ShaderExportConstantAllocation out;
    out.buffer = segment.buffer;
    out.staging = static_cast<uint8_t*>(segment.staging) + segment.head;
    out.offset = segment.head;

    // Offset head address
    segment.head += length;

    // OK
    return out;
}

void ShaderExportConstantAllocator::StageData(DeviceDispatchTable* table, VkCommandBuffer commandBuffer, VkBuffer resource, uint64_t offset, const void *data, size_t length) {
    // Copy over the data
    ShaderExportConstantAllocation hostAlloc = Allocate(table, length, 4u);
    std::memcpy(hostAlloc.staging, data, length);

    // Copy to device
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = length;
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(commandBuffer, hostAlloc.buffer, resource, 1u, &copyRegion);
}
