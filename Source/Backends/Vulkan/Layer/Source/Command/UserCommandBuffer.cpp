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
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ResourceState.h>

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

                // Shader Read/Write -> Transfer Write
                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
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

                // Transfer Write -> Shader Read/Write
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

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
            case CommandType::CopyBuffer: {
                auto* cmd = command.As<CopyBufferCommand>();

                // Get the data buffer
                VkBuffer sourceBuffer = device->dataHost->GetResourceBuffer(cmd->source);
                VkBuffer destBuffer   = device->dataHost->GetResourceBuffer(cmd->dest);

                VkBufferCopy copyRegion{};
                copyRegion.srcOffset = cmd->sourceOffset;
                copyRegion.dstOffset = cmd->destOffset;
                copyRegion.size = cmd->byteCount;

                device->commandBufferDispatchTable.next_vkCmdCopyBuffer(
                    commandBuffer,
                    sourceBuffer, destBuffer,
                    1u, &copyRegion
                );
                break;
            }
            case CommandType::SetResource: {
                auto *cmd = command.As<SetResourceCommand>();

                // Get the binding index
                uint32_t bindingIndex = device->dataHost->GetBindingIndex(state.shaderProgramID, cmd->id);

                // Lazy allocate
                if (bindingIndex >= state.shaderProgramBindings.Size()) {
                    state.shaderProgramBindings.Resize(bindingIndex + 1);
                }

                // Get state
                auto *resourceState = static_cast<BufferState*>(device->physicalResourceIdentifierMap.GetState(cmd->puid));

                // Lazily create a view for it
                if (!resourceState->bindingView) {
                    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
                    viewInfo.buffer = resourceState->object;
                    viewInfo.format = VK_FORMAT_R32_UINT;
                    viewInfo.range = VK_WHOLE_SIZE;
                    device->next_vkCreateBufferView(device->object, &viewInfo, nullptr, &resourceState->bindingView);
                }

                // Set binding
                state.shaderProgramBindings[bindingIndex] = UserBinding {
                    .bufferView = resourceState->bindingView
                };
                break;
            }
            case CommandType::SetResourceData: {
                auto *cmd = command.As<SetResourceDataCommand>();

                // Get the binding index
                uint32_t bindingIndex = device->dataHost->GetBindingIndex(state.shaderProgramID, cmd->id);

                // Lazy allocate
                if (bindingIndex >= state.shaderProgramBindings.Size()) {
                    state.shaderProgramBindings.Resize(bindingIndex + 1);
                }
                
                // Get the data buffer
                VkBufferView sourceBufferView = device->dataHost->GetResourceBufferView(cmd->buffer, VK_FORMAT_R32_UINT);

                // Set binding
                state.shaderProgramBindings[bindingIndex] = UserBinding {
                    .bufferView = sourceBufferView
                };
                break;
            }
            case CommandType::BeginPredicate: {
                auto* cmd = command.As<BeginPredicateCommand>();
                VkBuffer predicateBuffer = device->dataHost->GetResourceBuffer(cmd->buffer);

                // Generic barrier
                VkBufferMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
                barrier.buffer = predicateBuffer;
                barrier.offset = cmd->offset;
                barrier.size = sizeof(uint32_t);
                device->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    1, &barrier,
                    0, nullptr
                );
                
                // Standard, execute if set
                VkConditionalRenderingBeginInfoEXT beginInfo{VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT};
                beginInfo.buffer = predicateBuffer;
                beginInfo.offset = cmd->offset;
                
                device->commandBufferDispatchTable.next_vkCmdBeginConditionalRenderingEXT(
                    commandBuffer,
                    &beginInfo
                );
                break;
            }
            case CommandType::EndPredicate: {
                auto* cmd = command.As<EndPredicateCommand>();
                device->commandBufferDispatchTable.next_vkCmdEndConditionalRenderingEXT(commandBuffer);
                break;
            }
            case CommandType::Dispatch: 
            case CommandType::DispatchIndirect: {
                // Any resources to set?
                if (state.shaderProgramBindings.Size()) {
                    // Number of bindings
                    uint32_t bindingCount = 0;
                    device->dataHost->EnumerateProgram(state.shaderProgramID, &bindingCount, nullptr, ShaderDataType::BindingMask);

                    // Get all program bindings
                    std::vector<ShaderDataInfo> bindings(bindingCount);
                    device->dataHost->EnumerateProgram(state.shaderProgramID, &bindingCount, bindings.data(), ShaderDataType::BindingMask);

                    // We're expecting them all to be bound
                    ASSERT(bindingCount == state.shaderProgramBindings.Size(), "Unexpected binding count");

                    // Allocate the descriptor set dynamically
                    VkDescriptorSet allocation = streamState->freeDescriptorAllocator.Allocate(device->shaderProgramHost->GetDescriptorSetLayout(state.shaderProgramID));
                    {
                        TrivialStackVector<VkWriteDescriptorSet, 4u> vkWriteDescriptorSet;

                        // Populate all bindings
                        for (size_t i = 0; i < bindingCount; i++) {
                            UserBinding &binding = state.shaderProgramBindings[i];

                            // Shader wise data info
                            const ShaderDataInfo& dataInfo = bindings[i];
                            ASSERT(dataInfo.bufferBinding.format == Backend::IL::Format::R32UInt, "Unsupported type");
                            
                            // Storage?
                            if (dataInfo.bufferBinding.isWritable) {
                                vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
                                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    .dstSet = allocation,
                                    .dstBinding = static_cast<uint32_t>(i),
                                    .descriptorCount = 1,
                                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                    .pTexelBufferView = &binding.bufferView
                                });
                            } else {
                                vkWriteDescriptorSet.Add(VkWriteDescriptorSet {
                                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                    .dstSet = allocation,
                                    .dstBinding = static_cast<uint32_t>(i),
                                    .descriptorCount = 1,
                                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                                    .pTexelBufferView = &binding.bufferView
                                });
                            }
                        }

                        // Finally, update the descriptor set
                        device->next_vkUpdateDescriptorSets(
                            device->object,
                            static_cast<uint32_t>(vkWriteDescriptorSet.Size()), vkWriteDescriptorSet.Data(),
                            0u, nullptr
                        );
                    }

                    // Bind the set
                    device->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
                        commandBuffer,
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        device->shaderProgramHost->GetPipelineLayout(state.shaderProgramID),
                        1, 1, &allocation,
                        0, nullptr
                    );

                    // Clear last bindings
                    state.shaderProgramBindings.Clear();
                }
                
                // Invoke
                if (static_cast<CommandType>(command.commandType) == CommandType::DispatchIndirect) {
                    auto* cmd = command.As<DispatchIndirectCommand>();

                    VkBuffer indirectBuffer = device->dataHost->GetResourceBuffer(cmd->buffer);

                    // Invoke program
                    device->commandBufferDispatchTable.next_vkCmdDispatchIndirect(
                        commandBuffer,
                        indirectBuffer,
                        cmd->offset
                    );
                } else {
                    auto* cmd = command.As<DispatchCommand>();

                    // Invoke program
                    device->commandBufferDispatchTable.next_vkCmdDispatch(
                        commandBuffer,
                        cmd->groupCountX,
                        cmd->groupCountY,
                        cmd->groupCountZ
                    );
                }
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
    ReconstructState(device, commandBuffer, streamState, state.reconstructionFlags);
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