#include <Backends/Vulkan/Command/UserCommandBuffer.h>
#include <Backends/Vulkan/Command/UserCommandState.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/ShaderProgram/ShaderProgramHost.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>

// Common
#include "Common/Enum.h"

static void ReconstructPipelineState(CommandBufferObject* commandBuffer, const UserCommandState& state) {
    ShaderExportPipelineBindState& bindState = commandBuffer->streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Compute)];

    // Bind the expected pipeline
    if (bindState.pipeline) {
        commandBuffer->dispatchTable.next_vkCmdBindPipeline(commandBuffer->object, VK_PIPELINE_BIND_POINT_COMPUTE, bindState.pipelineObject);

        // Rebind the export, invalidated by layout compatibility
        commandBuffer->table->exportStreamer->BindShaderExport(commandBuffer->streamState, bindState.pipeline, commandBuffer);
    }

    // Rebind all expected states
    for (uint32_t i = 0; i < bindState.pipeline->layout->boundUserDescriptorStates; i++) {
        const ShaderExportDescriptorState& descriptorState = bindState.persistentDescriptorState.at(i);

        // Invalid or mismatched hash?
        if (!descriptorState.set || bindState.pipeline->layout->compatabilityHashes[i] != descriptorState.compatabilityHash) {
            continue;
        }

        // Bind the expected set
        commandBuffer->dispatchTable.next_vkCmdBindDescriptorSets(
            commandBuffer->object,
            VK_PIPELINE_BIND_POINT_COMPUTE, bindState.pipeline->layout->object,
            i, 1u, &descriptorState.set,
            descriptorState.dynamicOffsets.count, descriptorState.dynamicOffsets.data
        );
    }
}

static void ReconstructPushConstantState(CommandBufferObject* commandBuffer, const UserCommandState& state) {
    ShaderExportPipelineBindState& bindState = commandBuffer->streamState->pipelineBindPoints[static_cast<uint32_t>(PipelineType::Compute)];

    // Relevant bind state?
    if (!bindState.pipeline || bindState.pipeline->layout->dataPushConstantLength == 0) {
        return;
    }

    // Reconstruct the push constant data
    commandBuffer->dispatchTable.next_vkCmdPushConstants(
        commandBuffer->object,
        bindState.pipeline->layout->object,
        bindState.pipeline->layout->pushConstantRangeMask,
        0u,
        bindState.pipeline->layout->userPushConstantLength,
        commandBuffer->streamState->persistentPushConstantData.data()
    );
}

static void ReconstructState(CommandBufferObject* commandBuffer, const UserCommandState& state) {
    if (state.reconstructionFlags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(commandBuffer, state);
    }

    if (state.reconstructionFlags & ReconstructionFlag::PushConstant) {
        ReconstructPushConstantState(commandBuffer, state);
    }
}

void CommitCommands(CommandBufferObject* commandBuffer) {
    UserCommandState state;

    // Handle all commands
    for (const Command& command : commandBuffer->userContext.buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            case CommandType::SetShaderProgram: {
                auto* cmd = command.As<SetShaderProgramCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::Pipeline;
                state.shaderProgramID = cmd->id;

                // Get pipeline
                VkPipeline pipeline = commandBuffer->table->shaderProgramHost->GetPipeline(cmd->id);

                // Get layout
                VkPipelineLayout layout = commandBuffer->table->shaderProgramHost->GetPipelineLayout(cmd->id);

                // Bind pipeline
                commandBuffer->dispatchTable.next_vkCmdBindPipeline(
                    commandBuffer->object,
                    VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipeline
                );

                // Bind shader export
                commandBuffer->table->exportStreamer->BindShaderExport(
                    commandBuffer->streamState,
                    PipelineType::Compute,
                    layout,
                    pipeline,
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
                VkPipelineLayout layout = commandBuffer->table->shaderProgramHost->GetPipelineLayout(state.shaderProgramID);

                // Get push constant offset
                uint32_t offset = commandBuffer->table->eventRemappingTable[cmd->id];

                // Push constants
                commandBuffer->dispatchTable.next_vkCmdPushConstants(
                    commandBuffer->object,
                    layout,
                    VK_SHADER_STAGE_COMPUTE_BIT,
                    offset,
                    sizeof(uint32_t),
                    &cmd->value
                );
                break;
            }
            case CommandType::Dispatch: {
                auto* cmd = command.As<DispatchCommand>();

                // Invoke program
                commandBuffer->dispatchTable.next_vkCmdDispatch(
                    commandBuffer->object,
                    cmd->groupCountX,
                    cmd->groupCountY,
                    cmd->groupCountZ
                );

                // Generic shader UAV barrier
                VkMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                commandBuffer->dispatchTable.next_vkCmdPipelineBarrier(
                    commandBuffer->object,
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

    // Done
    commandBuffer->userContext.buffer.Clear();

    // Reconstruct expected user state
    ReconstructState(commandBuffer, state);
}
