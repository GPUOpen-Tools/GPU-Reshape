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

#include <Backends/Vulkan/Raytracing.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Export/StreamState.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Command/UserCommandBuffer.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Programs/Programs.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/RaytracingPipelineState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Shared
#include <Shared/ShaderRecordPatching.h>

struct RaytracingCacheInfo {
    VkStridedDeviceAddressRegionKHR raygenShaderBindingTable;
    VkStridedDeviceAddressRegionKHR missShaderBindingTable;
    VkStridedDeviceAddressRegionKHR hitShaderBindingTable;
    VkStridedDeviceAddressRegionKHR callableShaderBindingTable;
};

static ShaderExportDeviceAllocation CopyRaytracingRange(CommandBufferObject* commandBuffer, BufferState* bufferState, const VkStridedDeviceAddressRegionKHR& range) {
    ShaderExportDeviceAllocation sourceAllocation = commandBuffer->streamState->deviceAllocator.Allocate(commandBuffer->table, range.size);

    // Copy region, assume offset from address
    VkBufferCopy copyRegion;
    copyRegion.srcOffset = range.deviceAddress - bufferState->virtualAddress;
    copyRegion.dstOffset = 0;
    copyRegion.size = range.size;

    // Copy over the range
    commandBuffer->dispatchTable.next_vkCmdCopyBuffer(
        commandBuffer->object,
        bufferState->object,
        sourceAllocation.buffer,
        1,
        &copyRegion
    );

    // Generic barrier
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    commandBuffer->dispatchTable.next_vkCmdPipelineBarrier(
        commandBuffer->object,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0x0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    // OK
    return sourceAllocation;
}

static VkStridedDeviceAddressRegionKHR PatchRaytracingIdentifiersRangeImmediate(CommandBufferObject* commandBuffer, const RaytracingPipelineState* pipeline, RaytracingShaderIdentifierPatch* patchTable, const VkStridedDeviceAddressRegionKHR& range) {
    auto& program = commandBuffer->table->programs->raytracing;

    // If there's no valid range, just skip it entirely
    if (!range.deviceAddress || !range.size) {
        return range;
    }

    // Find the buffer from the virtual address
    BufferState *bufferState = commandBuffer->table->virtualAddressTable.Find(range.deviceAddress);
    ASSERT(bufferState, "Failed to associate device address");

    // Copy over the source SBT range
    // We don't actually need to do this, we could copy it immediately into the patched range
    // Also fairly allocation heavy, consider a shared allocation
    ShaderExportDeviceAllocation sourceAllocation = CopyRaytracingRange(commandBuffer, bufferState, range);

    // Allocate the patched SBT
    ShaderExportDeviceAllocation patchAllocation = commandBuffer->streamState->deviceAllocator.Allocate(commandBuffer->table, range.size);

    // Total number of records
    const uint32_t recordCount = static_cast<uint32_t>(range.size / range.stride);

    // Allocate the patch constant data
    ShaderExportConstantAllocation constantAllocation = commandBuffer->streamState->constantAllocator.Allocate(commandBuffer->table, sizeof(SBTPatchConstantData));

    // Setup the constant data
    auto* sbtPatchData = static_cast<SBTPatchConstantData*>(constantAllocation.staging);
    sbtPatchData->SBTRecordCount = recordCount;
    sbtPatchData->IdentifierDWordStride = static_cast<uint32_t>(range.stride / sizeof(uint32_t));
    sbtPatchData->NativeIdentifierDWordStride = commandBuffer->table->physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize / sizeof(uint32_t);
    sbtPatchData->IdentifierHandleDWordStride = (commandBuffer->table->physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize + sizeof(SBTShaderGroupIdentifierEmbeddedData)) / sizeof(uint32_t);

    // Allocate the descriptor set
    VkDescriptorSet descriptorSet = commandBuffer->streamState->freeDescriptorAllocator.Allocate(program.sbtPatchSetLayout);
    {
        TrivialStackVector<VkWriteDescriptorSet, 4u> vkWriteDescriptorSet;

        // Constant data
        VkDescriptorBufferInfo constantBufferInfo{};
        constantBufferInfo.buffer = constantAllocation.buffer;
        constantBufferInfo.offset = constantAllocation.offset;
        constantBufferInfo.range = sizeof(SBTPatchConstantData);
        vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &constantBufferInfo
        });

        // Source SBT
        VkDescriptorBufferInfo sourceDWordBufferInfo{};
        sourceDWordBufferInfo.buffer = sourceAllocation.buffer;
        sourceDWordBufferInfo.range = range.size;
        vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &sourceDWordBufferInfo
        });

        // Patched SBT
        VkDescriptorBufferInfo patchDWordBufferInfo{};
        patchDWordBufferInfo.buffer = patchAllocation.buffer;
        patchDWordBufferInfo.range = patchAllocation.length;
        vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &patchDWordBufferInfo
        });

        // Patched identifiers
        VkDescriptorBufferInfo patchIdentifierDWords{};
        patchIdentifierDWords.buffer = patchTable->buffer;
        patchIdentifierDWords.range = pipeline->identifierSet.count * commandBuffer->table->physicalDeviceRayTracingPipelineProperties.shaderGroupHandleSize;
        vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 3,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &patchIdentifierDWords
        });

        // Finally, update the descriptor set
        commandBuffer->table->next_vkUpdateDescriptorSets(
            commandBuffer->table->object,
            static_cast<uint32_t>(vkWriteDescriptorSet.Size()), vkWriteDescriptorSet.Data(),
            0u, nullptr
        );
    }

    // Bind and dispatch the patch program
    commandBuffer->dispatchTable.next_vkCmdBindPipeline(commandBuffer->object, VK_PIPELINE_BIND_POINT_COMPUTE, program.sbtPatchPipeline);
    commandBuffer->dispatchTable.next_vkCmdBindDescriptorSets(commandBuffer->object, VK_PIPELINE_BIND_POINT_COMPUTE, program.sbtPatchPipelineLayout, 0u, 1u, &descriptorSet, 0u, nullptr);
    commandBuffer->dispatchTable.next_vkCmdDispatch(commandBuffer->object, (recordCount + 31) / 32, 1, 1);

    // Copy over the old range
    VkStridedDeviceAddressRegionKHR region = range;

    // Replace the address by the patched
    VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer = patchAllocation.buffer;
    region.deviceAddress = commandBuffer->table->next_vkGetBufferDeviceAddress(commandBuffer->table->object, &info);

    // Useful for debugging bad patching
#if 0
    AddDebugStream(commandBuffer, bufferState->object, range.deviceAddress - bufferState->virtualAddress, patchAllocation.length, "SourceAllocation");
    AddDebugStream(commandBuffer, patchAllocation.buffer, 0, patchAllocation.length, "PatchAllocation");
#endif // 0

    return region;
}

RaytracingCacheInfo PatchRaytracingIdentifiersImmediate(CommandBufferObject* commandBuffer, const RaytracingCacheInfo& info) {
    ShaderExportPipelineBindState &bindPoint = commandBuffer->streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Raytracing)];

    ASSERT(bindPoint.pipeline->type == PipelineType::Raytracing, "Unexpected pipeline type");
    auto* pipeline = static_cast<const RaytracingPipelineState*>(bindPoint.pipeline);

    // Get the current hot patch table
    // TODO[rt]: Dont keep a current patch table, just index by the current hash index instead somehow
    RaytracingShaderIdentifierPatch *patchTable = pipeline->hotSwapPatchTable.load();
    if (!patchTable) {
        return info;
    }

    // Patch all ranges individually
    RaytracingCacheInfo patch{};
    patch.raygenShaderBindingTable = PatchRaytracingIdentifiersRangeImmediate(commandBuffer, pipeline, patchTable, info.raygenShaderBindingTable);
    patch.missShaderBindingTable = PatchRaytracingIdentifiersRangeImmediate(commandBuffer, pipeline, patchTable, info.missShaderBindingTable);
    patch.hitShaderBindingTable = PatchRaytracingIdentifiersRangeImmediate(commandBuffer, pipeline, patchTable, info.hitShaderBindingTable);
    patch.callableShaderBindingTable = PatchRaytracingIdentifiersRangeImmediate(commandBuffer, pipeline, patchTable, info.callableShaderBindingTable);

    // Reconstruct the previous raytracing command buffer state
    ReconstructState(commandBuffer->table, commandBuffer->object, commandBuffer->streamState, ReconstructionFlag::Pipeline);

    // OK
    return patch;
}

static void CommitRaytracing(CommandBufferObject* commandBuffer) {
    DeviceDispatchTable* table = commandBuffer->table;

    // Commit all commands prior to binding
    CommitCommands(commandBuffer);

    // Inform the streamer
    table->exportStreamer->Commit(commandBuffer->streamState, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, commandBuffer->object);

    // TODO: Update the event data in batches
    // Note: We consider compute to be raytracing
    if (uint64_t bitMask = commandBuffer->userContext.eventStack.GetComputeDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            // Push the event data
            commandBuffer->dispatchTable.next_vkCmdPushConstants(
                commandBuffer->object,
                commandBuffer->context.pipeline->layout->object,
                VK_SHADER_STAGE_ALL,
                commandBuffer->context.pipeline->layout->dataPushConstantOffset + index,
                sizeof(uint32_t),
                commandBuffer->userContext.eventStack.GetData() + index
            );

            // Next!
            bitMask &= ~(1ull << index);
        }

        // Cleanup
        commandBuffer->userContext.eventStack.FlushCompute();
    }
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdTraceRaysKHR(CommandBufferObject* commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth) {
    ShaderExportPipelineBindState &bindPoint = commandBuffer->streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Raytracing)];

    // To info
    RaytracingCacheInfo info;
    info.raygenShaderBindingTable = *pRaygenShaderBindingTable;
    info.missShaderBindingTable = *pMissShaderBindingTable;
    info.hitShaderBindingTable = *pHitShaderBindingTable;
    info.callableShaderBindingTable = *pCallableShaderBindingTable;

    // If instrumented, get the patched identifiers
    if (bindPoint.pipelineInstrument) {
        info = PatchRaytracingIdentifiersImmediate(commandBuffer, info);
    }

    // Append execution info, if used
    if (UsesExecutionInfo(commandBuffer, PipelineType::Raytracing)) {
        ExecutionInfo execInfo = GetBaseExecutionInfo(commandBuffer, PipelineType::Raytracing);
        execInfo.executionFlags = ExecutionFlag::TypeRaytracing;
        execInfo.dispatch.groupCountX = width;
        execInfo.dispatch.groupCountY = height;
        execInfo.dispatch.groupCountZ = depth;
        commandBuffer->table->exportStreamer->SetExecutionInfo(commandBuffer->streamState, commandBuffer->object, PipelineType::Raytracing, execInfo);
    }
    
    // Commit all commands
    CommitRaytracing(commandBuffer);
    
    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdTraceRaysKHR(
        commandBuffer->object,
        &info.raygenShaderBindingTable,
        &info.missShaderBindingTable,
        &info.hitShaderBindingTable,
        &info.callableShaderBindingTable,
        width, height, depth
    );
}
