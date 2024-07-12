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

#include <Backends/Vulkan/Command/UserCommandBuffer.h>
#include <Backends/Vulkan/Command/UserCommandState.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/RenderPassState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/ShaderProgram/ShaderProgramHost.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>

static void ReconstructPipelineState(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, ShaderExportStreamState* streamState, const UserCommandState& state) {
    ShaderExportPipelineBindState& bindState = streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Compute)];

    // Bind the expected pipeline
    if (bindState.pipeline) {
        device->commandBufferDispatchTable.next_vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindState.pipelineObject);

        // Rebind the export, invalidated by layout compatibility
        device->exportStreamer->BindShaderExport(streamState, bindState.pipeline, commandBuffer);

        // Rebind all expected states
        for (uint32_t i = 0; i < bindState.pipeline->layout->boundUserDescriptorStates; i++) {
            const ShaderExportDescriptorState &descriptorState = bindState.persistentDescriptorState.at(i);

            // Invalid or mismatched hash?
            if (!descriptorState.set || bindState.pipeline->layout->compatabilityHashes[i] != descriptorState.compatabilityHash) {
                continue;
            }

            // Bind the expected set
            device->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_COMPUTE, bindState.pipeline->layout->object,
                i, 1u, &descriptorState.set,
                descriptorState.dynamicOffsets.count, descriptorState.dynamicOffsets.data);
        }
    }
}

static void ReconstructPushConstantState(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, ShaderExportStreamState* streamState, const UserCommandState& state) {
    ShaderExportPipelineBindState& bindState = streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Compute)];

    // Relevant bind state?
    if (!bindState.pipeline || bindState.pipeline->layout->dataPushConstantLength == 0) {
        return;
    }

    // Reconstruct the push constant data
    device->commandBufferDispatchTable.next_vkCmdPushConstants(
        commandBuffer,
        bindState.pipeline->layout->object,
        bindState.pipeline->layout->pushConstantRangeMask,
        0u,
        bindState.pipeline->layout->userPushConstantLength,
        streamState->persistentPushConstantData.data()
    );
}

static void ReconstructRenderPassState(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, ShaderExportStreamState* streamState, const UserCommandState& state) {
    // Use the reconstruction object instead of native
    VkRenderPassBeginInfo beginInfo = streamState->renderPass.deepCopy.createInfo;
    beginInfo.renderPass = device->states_renderPass.Get(beginInfo.renderPass)->reconstructionObject;
    
    // Reconstruct render pass
    device->commandBufferDispatchTable.next_vkCmdBeginRenderPass(
        commandBuffer,
        &beginInfo,
        streamState->renderPass.subpassContents
    );
}

static void ReconstructState(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, ShaderExportStreamState* streamState, const UserCommandState& state) {
    if (state.reconstructionFlags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(device, commandBuffer, streamState, state);
    }

    if (state.reconstructionFlags & ReconstructionFlag::PushConstant) {
        ReconstructPushConstantState(device, commandBuffer, streamState, state);
    }

    if (state.reconstructionFlags & ReconstructionFlag::RenderPass) {
        ReconstructRenderPassState(device, commandBuffer, streamState, state);
    }
}

void CommitCommands(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, const CommandBuffer& buffer, ShaderExportStreamState* streamState) {
    UserCommandState state;

    // Always end the current render pass if any commands
    if (buffer.Count() && streamState->renderPass.insideRenderPass) {
        device->commandBufferDispatchTable.next_vkCmdEndRenderPass(commandBuffer);
        state.reconstructionFlags |= ReconstructionFlag::RenderPass;
    }

    // Handle all commands
    for (const Command& command : buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            default: {
                ASSERT(false, "Invalid command for target");
                break;
            }
            case CommandType::SetShaderProgram: {
                auto* cmd = command.As<SetShaderProgramCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::Pipeline;
                state.shaderProgramID = cmd->id;

                // Get pipeline
                VkPipeline pipeline = device->shaderProgramHost->GetPipeline(cmd->id);

                // Get layout
                VkPipelineLayout layout = device->shaderProgramHost->GetPipelineLayout(cmd->id);

                // Bind pipeline
                device->commandBufferDispatchTable.next_vkCmdBindPipeline(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline
                );

                // Bind shader export
                device->exportStreamer->BindShaderExport(
                    streamState,
                    PipelineType::Compute,
                    layout,
                    pipeline,
                    0u,
                    0u,
                    commandBuffer
                );
                break;
            }
            case CommandType::SetEventData: {
                auto* cmd = command.As<SetEventDataCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::PushConstant;

                // Get current layout
                VkPipelineLayout layout = device->shaderProgramHost->GetPipelineLayout(state.shaderProgramID);

                // Get push constant offset
                uint32_t offset = device->eventRemappingTable[cmd->id];

                // Push constants
                device->commandBufferDispatchTable.next_vkCmdPushConstants(
                    commandBuffer,
                    layout,
                    VK_SHADER_STAGE_ALL,
                    offset,
                    sizeof(uint32_t),
                    &cmd->value
                );
                break;
            }
            case CommandType::SetDescriptorData: {
                auto* cmd = command.As<SetDescriptorDataCommand>();

                // Get offset
                uint32_t dwordOffset = device->constantRemappingTable[cmd->id];
                uint32_t length = cmd->commandSize - sizeof(SetDescriptorDataCommand);

                // Shader Read -> Transfer Write
                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = streamState->constantShaderDataBuffer.buffer;
                barrier.offset = sizeof(uint32_t) * dwordOffset;
                barrier.size = length;

                // Stall the pipeline
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );

                // Update the buffer with inline command buffer storage (simplifies my life)
                device->commandBufferDispatchTable.next_vkCmdUpdateBuffer(
                    commandBuffer,
                    streamState->constantShaderDataBuffer.buffer,
                    sizeof(uint32_t) * dwordOffset,
                    length,
                    reinterpret_cast<const uint8_t*>(cmd) + sizeof(SetDescriptorDataCommand)
                );

                // Transfer Write -> Shader Read
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                // Stall the pipeline
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );
                break;
            }
            case CommandType::StageBuffer: {
                auto* cmd = command.As<StageBufferCommand>();

                // Get the data buffer
                VkBuffer resourceBuffer = device->dataHost->GetResourceBuffer(cmd->id);

                // Actual length of the data
                size_t length = cmd->commandSize - sizeof(StageBufferCommand);

                // Shader Read -> Transfer Write
                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.buffer = resourceBuffer;
                barrier.offset = cmd->offset;
                barrier.size = length;

                // Stall the pipeline
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );

                // Update the buffer with inline command buffer storage
                device->commandBufferDispatchTable.next_vkCmdUpdateBuffer(
                    commandBuffer,
                    resourceBuffer,
                    cmd->offset,
                    length,
                    reinterpret_cast<const uint8_t*>(cmd) + sizeof(StageBufferCommand)
                );

                // Transfer Write -> Shader Read
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                // Stall the pipeline
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );
                break;
            }
            case CommandType::ClearBuffer: {
                auto* cmd = command.As<ClearBufferCommand>();

                // Get the data buffer
                VkBuffer resourceBuffer = device->dataHost->GetResourceBuffer(cmd->id);

                // Fill the range with zero's
                device->commandBufferDispatchTable.next_vkCmdFillBuffer(
                    commandBuffer,
                    resourceBuffer,
                    cmd->offset,
                    cmd->length,
                    cmd->value
                );
                break;
            }
            case CommandType::Dispatch: {
                auto* cmd = command.As<DispatchCommand>();

                // Invoke program
                device->commandBufferDispatchTable.next_vkCmdDispatch(
                    commandBuffer,
                    cmd->groupCountX,
                    cmd->groupCountY,
                    cmd->groupCountZ
                );
                break;
            }
            case CommandType::UAVBarrier: {
                // Generic shader UAV barrier
                VkMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    1, &barrier,
                    0, nullptr,
                    0, nullptr
                );
                break;
            }
        }
    }


    // Reconstruct expected user state
    ReconstructState(device, commandBuffer, streamState, state);
}

void CommitCommands(CommandBufferObject* commandBuffer) {
    // Commit all commands
    CommitCommands(
        commandBuffer->table,
        commandBuffer->object,
        commandBuffer->userContext.buffer,
        commandBuffer->streamState
    );

    // Done
    commandBuffer->userContext.buffer.Clear();
}