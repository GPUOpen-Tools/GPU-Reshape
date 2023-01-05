#include <Backends/DX12/Command/UserCommandBuffer.h>
#include <Backends/DX12/Command/UserCommandState.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/ShaderProgram/ShaderProgramHost.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Table.Gen.h>

// Common
#include "Common/Enum.h"

static void ReconstructPipelineState(CommandListState* commandList, DeviceState* device, const UserCommandState& state) {
    ShaderExportStreamBindState& bindState = commandList->streamState->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Any?
    if (!commandList->streamState->pipelineObject) {
        return;
    }

    // Reset signature if needed
    if (commandList->streamState->pipeline->type == PipelineType::Compute) {
        commandList->object->SetComputeRootSignature(commandList->streamState->pipeline->signature->object);
    }

    // Set PSO
    commandList->object->SetPipelineState(commandList->streamState->pipelineObject);

    // Rebind root data, invalidated by signature change
    if (commandList->streamState->pipeline->type == PipelineType::Compute) {
        for (uint32_t i = 0; i < commandList->streamState->pipeline->signature->userRootCount; i++) {
            const ShaderExportRootParameterValue& value = bindState.persistentRootParameters[i];

            switch (value.type) {
                case ShaderExportRootParameterValueType::None:
                    break;
                case ShaderExportRootParameterValueType::Descriptor:
                    commandList->object->SetComputeRootDescriptorTable(i, value.payload.descriptor);
                    break;
                case ShaderExportRootParameterValueType::SRV:
                    commandList->object->SetComputeRootShaderResourceView(i, value.payload.virtualAddress);
                    break;
                case ShaderExportRootParameterValueType::UAV:
                    commandList->object->SetComputeRootUnorderedAccessView(i, value.payload.virtualAddress);
                    break;
                case ShaderExportRootParameterValueType::CBV:
                    commandList->object->SetComputeRootConstantBufferView(i, value.payload.virtualAddress);
                    break;
                case ShaderExportRootParameterValueType::Constant:
                    commandList->object->SetComputeRoot32BitConstants(
                        i,
                        value.payload.constant.dataByteCount,
                        value.payload.constant.data,
                        0
                    );
                    break;
            }
        }
    }

    // Rebind the export, invalidated by signature change
    device->exportStreamer->BindShaderExport(commandList->streamState, commandList->streamState->pipeline, commandList);
}

static void ReconstructState(CommandListState* commandList, DeviceState* device, const UserCommandState& state) {
    if (state.reconstructionFlags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(commandList, device, state);
    }
}

void CommitCommands(CommandListState* commandList) {
    UserCommandState state;

    // Get device
    auto device = GetTable(commandList->parent);

    // Handle all commands
    for (const Command& command : commandList->userContext.buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            case CommandType::SetShaderProgram: {
                auto* cmd = command.As<SetShaderProgramCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::Pipeline;
                state.shaderProgramID = cmd->id;

                // Set pipeline
                commandList->object->SetComputeRootSignature(device.state->shaderProgramHost->GetSignature());
                commandList->object->SetPipelineState(device.state->shaderProgramHost->GetPipeline(cmd->id));

                // Bind global shader export
                device.state->exportStreamer->BindShaderExport(commandList->streamState, 0u, PipelineType::Compute, commandList);
                break;
            }
            case CommandType::SetEventData: {
                auto* cmd = command.As<SetEventDataCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::RootConstant;

                // Get offset
                uint32_t offset = device.state->eventRemappingTable[cmd->id];

                // Bind root data
                commandList->object->SetComputeRoot32BitConstant(
                    2u,
                    cmd->value,
                    offset
                );
                break;
            }
            case CommandType::Dispatch: {
                auto* cmd = command.As<DispatchCommand>();

                // Invoke
                commandList->object->Dispatch(
                    cmd->groupCountX,
                    cmd->groupCountY,
                    cmd->groupCountZ
                );

                // Assumed barrier for simplicity
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                commandList->object->ResourceBarrier(1u, &barrier);
                break;
            }
        }
    }

    // Reconstruct user state
    ReconstructState(commandList, device.state, state);
}
