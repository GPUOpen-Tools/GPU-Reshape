#include <Backends/DX12/Command/UserCommandBuffer.h>
#include <Backends/DX12/Command/UserCommandState.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/ShaderProgram/ShaderProgramHost.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/Table.Gen.h>

// Common
#include "Common/Enum.h"

static void ReconstructPipelineState(CommandListState *commandList, DeviceState *device, const UserCommandState &state) {
    ShaderExportStreamBindState &bindState = commandList->streamState->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Any?
    if (!commandList->streamState->pipeline) {
        return;
    }

    // Reset signature if needed
    if (bindState.rootSignature) {
        commandList->object->SetComputeRootSignature(bindState.rootSignature->object);
    }

    // Set PSO
    if (commandList->streamState->pipelineObject) {
        commandList->object->SetPipelineState(commandList->streamState->pipelineObject);
    } else {
        commandList->object->SetPipelineState(commandList->streamState->pipeline->object);
    }

    // Rebind root data, invalidated by signature change
    if (bindState.rootSignature) {
        for (uint32_t i = 0; i < bindState.rootSignature->userRootCount; i++) {
            const ShaderExportRootParameterValue &value = bindState.persistentRootParameters[i];

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
                        value.payload.constant.dataByteCount / sizeof(uint32_t),
                        value.payload.constant.data,
                        0
                    );
                    break;
            }
        }
    }

    // Compute overwritten at this point
    commandList->streamState->pipelineSegmentMask &= ~PipelineTypeSet(PipelineType::Compute);

    // Rebind the export, invalidated by signature change
    device->exportStreamer->BindShaderExport(commandList->streamState, commandList->streamState->pipeline, commandList);
}

static void ReconstructState(CommandListState *commandList, DeviceState *device, const UserCommandState &state) {
    if (state.reconstructionFlags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(commandList, device, state);
    }
}

void CommitCommands(CommandListState *commandList) {
    UserCommandState state;

    // Get device
    auto device = GetTable(commandList->parent);

    // Handle all commands
    for (const Command &command: commandList->userContext.buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            case CommandType::SetShaderProgram: {
                auto *cmd = command.As<SetShaderProgramCommand>();

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
                auto *cmd = command.As<SetEventDataCommand>();

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
            case CommandType::SetDescriptorData: {
                auto *cmd = command.As<SetDescriptorDataCommand>();

                ConstantShaderDataBuffer &constantBuffer = commandList->streamState->constantShaderDataBuffer;

                // Get offset
                uint32_t dwordOffset = device.state->constantRemappingTable[cmd->id];
                uint32_t dwordCount = 1u;

                // Shader Read -> Copy Dest
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = commandList->streamState->constantShaderDataBuffer.allocation.resource;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                commandList->object->ResourceBarrier(1u, &barrier);

                // Needs a staging roll?
                if (constantBuffer.staging.empty() || constantBuffer.staging.back().CanAccomodate(dwordCount * sizeof(uint32_t))) {
                    // Next byte count
                    const size_t lastByteCount = constantBuffer.staging.empty() ? 16'384 : constantBuffer.staging.back().size;
                    const size_t byteCount = std::max<size_t>(dwordCount * sizeof(uint32_t), lastByteCount) * 1.5f;

                    // Mapped description
                    D3D12_RESOURCE_DESC desc{};
                    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                    desc.Alignment = 0;
                    desc.Width = byteCount;
                    desc.Height = 1;
                    desc.DepthOrArraySize = 1;
                    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                    desc.Format = DXGI_FORMAT_UNKNOWN;
                    desc.MipLevels = 1;
                    desc.SampleDesc.Quality = 0;
                    desc.SampleDesc.Count = 1;

                    // Allocate buffer data on host, let the drivers handle page swapping
                    ConstantShaderDataStaging& staging = constantBuffer.staging.emplace_back();
                    staging.allocation = device.state->deviceAllocator->Allocate(desc, AllocationResidency::Host);
                    staging.size = desc.Width;
                }

                // Assume last staging
                ConstantShaderDataStaging& staging = constantBuffer.staging.back();

                // Opaque data
                void* mapped{nullptr};

                // Map effective range
                D3D12_RANGE range{};
                range.Begin = dwordOffset * sizeof(uint32_t);
                range.End = (dwordOffset + dwordCount) * sizeof(uint32_t);
                staging.allocation.resource->Map(0u, &range, &mapped);

                // Update data
                std::memcpy(mapped, &cmd->value, sizeof(uint32_t));

                // Unmap
                staging.allocation.resource->Unmap(0u, &range);

                // Copy from staging
                commandList->object->CopyBufferRegion(
                    commandList->streamState->constantShaderDataBuffer.allocation.resource,
                    dwordOffset * sizeof(uint32_t),
                    staging.allocation.resource,
                    staging.head,
                    dwordCount * sizeof(uint32_t)
                );

                // Next!
                staging.head += dwordCount * sizeof(uint32_t);

                // Copy Dest -> Shader Read
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                commandList->object->ResourceBarrier(1u, &barrier);
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
    
    // Done
    commandList->userContext.buffer.Clear();
    
    // Reconstruct user state
    ReconstructState(commandList, device.state, state);
}
