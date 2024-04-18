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

#include <Backends/DX12/Command/UserCommandBuffer.h>
#include <Backends/DX12/Command/UserCommandState.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/ShaderProgram/ShaderProgramHost.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/RenderPass.h>
#include <Backends/DX12/Table.Gen.h>

// Common
#include "Common/Enum.h"

static void ReconstructPipelineState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState, const UserCommandState &state) {
    ShaderExportStreamBindState &bindState = streamState->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Reset signature if needed
    if (bindState.rootSignature) {
        commandList->SetComputeRootSignature(bindState.rootSignature->object);
    }

    // Set PSO if needed
    if (streamState->pipelineObject) {
        commandList->SetPipelineState(streamState->pipelineObject);
    } else if (streamState->pipeline) {
        commandList->SetPipelineState(streamState->pipeline->object);
    }
    
    // Reset root data if needed, invalidated by signature change
    if (bindState.rootSignature) {
        for (uint32_t i = 0; i < bindState.rootSignature->logicalMapping.userRootCount; i++) {
            const ShaderExportRootParameterValue &value = bindState.persistentRootParameters[i];

            // Get the expected heap type
            D3D12_DESCRIPTOR_HEAP_TYPE heapType = bindState.rootSignature->logicalMapping.userRootHeapTypes[i];
            
            switch (value.type) {
                case ShaderExportRootParameterValueType::None: {
                    break;
                }
                case ShaderExportRootParameterValueType::Descriptor: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Unexpected heap type");
                    commandList->SetComputeRootDescriptorTable(i, value.payload.descriptor);
                    break;
                }
                case ShaderExportRootParameterValueType::SRV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootShaderResourceView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::UAV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootUnorderedAccessView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::CBV: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Unexpected heap type");
                    commandList->SetComputeRootConstantBufferView(i, value.payload.virtualAddress);
                    break;
                }
                case ShaderExportRootParameterValueType::Constant: {
                    ASSERT(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, "Unexpected heap type");
                    commandList->SetComputeRoot32BitConstants(
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
        streamState->pipelineSegmentMask &= ~PipelineTypeSet(PipelineType::Compute);

        // Rebind the export, invalidated by signature change
        if (streamState->pipeline) {
            device->exportStreamer->BindShaderExport(streamState, streamState->pipeline, commandList);
        }
    }
}

static void ReconstructRenderPassState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState, const UserCommandState& state) {
    BeginRenderPassForReconstruction(static_cast<ID3D12GraphicsCommandList4*>(commandList), &streamState->renderPass);
}

static void ReconstructState(DeviceState *device, ID3D12GraphicsCommandList *commandList, ShaderExportStreamState* streamState, const UserCommandState &state) {
    if (state.reconstructionFlags & ReconstructionFlag::Pipeline) {
        ReconstructPipelineState(device, commandList, streamState, state);
    }

    if (state.reconstructionFlags & ReconstructionFlag::RenderPass) {
        ReconstructRenderPassState(device, commandList, streamState, state);
    }
}

void CommitCommands(DeviceState* device, ID3D12GraphicsCommandList* commandList, const CommandBuffer& buffer, ShaderExportStreamState* streamState) {
    UserCommandState state;

    // Always end the current render pass if any commands
    if (buffer.Count() && streamState->renderPass.insideRenderPass) {
        static_cast<ID3D12GraphicsCommandList4*>(commandList)->EndRenderPass();
        state.reconstructionFlags |= ReconstructionFlag::RenderPass;
    }
    
    // Handle all commands
    for (const Command &command: buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            case CommandType::SetShaderProgram: {
                auto *cmd = command.As<SetShaderProgramCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::Pipeline;
                state.shaderProgramID = cmd->id;

                // Set pipeline
                commandList->SetComputeRootSignature(device->shaderProgramHost->GetSignature());
                commandList->SetPipelineState(device->shaderProgramHost->GetPipeline(cmd->id));

                // Bind global shader export
                device->exportStreamer->BindShaderExport(streamState, 0u, PipelineType::Compute, commandList);
                break;
            }
            case CommandType::SetEventData: {
                auto *cmd = command.As<SetEventDataCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::RootConstant;

                // Get offset
                uint32_t offset = device->eventRemappingTable[cmd->id];

                // Bind root data
                commandList->SetComputeRoot32BitConstant(
                    2u,
                    cmd->value,
                    offset
                );
                break;
            }
            case CommandType::SetDescriptorData: {
                auto *cmd = command.As<SetDescriptorDataCommand>();

                // Expected read state
                D3D12_RESOURCE_STATES readState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

                // Get offset
                uint32_t dwordOffset = device->constantRemappingTable[cmd->id];
                uint32_t length = cmd->commandSize - sizeof(SetDescriptorDataCommand);

                // Shader Read -> Copy Dest
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = streamState->constantShaderDataBuffer.allocation.resource;
                barrier.Transition.StateBefore = readState;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                commandList->ResourceBarrier(1u, &barrier);

                // Allocate staging data
                ShaderExportConstantAllocation stagingAllocation = streamState->constantAllocator.Allocate(device->deviceAllocator, length);

                // Update data
                std::memcpy(stagingAllocation.staging, reinterpret_cast<const uint8_t*>(cmd) + sizeof(SetDescriptorDataCommand), length);

                // Copy from staging
                commandList->CopyBufferRegion(
                    streamState->constantShaderDataBuffer.allocation.resource,
                    dwordOffset * sizeof(uint32_t),
                    stagingAllocation.resource,
                    stagingAllocation.offset,
                    length
                );

                // Copy Dest -> Shader Read
                barrier.Transition.StateAfter = readState;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                commandList->ResourceBarrier(1u, &barrier);
                break;
            }
            case CommandType::StageBuffer: {
                auto *cmd = command.As<StageBufferCommand>();

                // Get the data allocation
                Allocation allocation = device->shaderDataHost->GetResourceAllocation(cmd->id);

                // Deduce allocation length
                size_t length = cmd->commandSize - sizeof(StageBufferCommand);

                // Allocate staging data
                ShaderExportConstantAllocation stagingAllocation = streamState->constantAllocator.Allocate(device->deviceAllocator, length);

                // Update data
                std::memcpy(stagingAllocation.staging, reinterpret_cast<const uint8_t*>(cmd) + sizeof(StageBufferCommand), length);

                // Using atomic copies?
                if (cmd->flags & StageBufferFlag::Atomic32) {
                    // TODO: Cache these somehow?
                    ID3D12GraphicsCommandList1* commandList1;
                    commandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList1), (void**)&commandList1);

                    // Resource dependencies
                    ID3D12Resource* dependencies[] = { allocation.resource };

                    // Given range
                    D3D12_SUBRESOURCE_RANGE_UINT64 atomicRange;
                    atomicRange.Subresource = 0;
                    atomicRange.Range.Begin = cmd->offset;
                    atomicRange.Range.End = cmd->offset + length;

                    // Perform an atomic copy from staging
                    commandList1->AtomicCopyBufferUINT(
                        allocation.resource,
                        cmd->offset,
                        stagingAllocation.resource,
                        stagingAllocation.offset,
                        1u,
                        dependencies,
                        &atomicRange
                    );

                    // Cleanup
                    commandList1->Release();
                } else {
                    // Copy from staging
                    commandList->CopyBufferRegion(
                        allocation.resource,
                        cmd->offset,
                        stagingAllocation.resource,
                        stagingAllocation.offset,
                        length
                    );
                }
                break;
            }
            case CommandType::Dispatch: {
                auto* cmd = command.As<DispatchCommand>();

                // Invoke
                commandList->Dispatch(
                    cmd->groupCountX,
                    cmd->groupCountY,
                    cmd->groupCountZ
                );
                break;
            }
            case CommandType::UAVBarrier: {
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                commandList->ResourceBarrier(1u, &barrier);
                break;
            }
        }
    }
    
    // Reconstruct user state
    ReconstructState(device, commandList, streamState, state);
}

void CommitCommands(CommandListState* state) {
    auto deviceTable = GetTable(state->parent);

    // Commit all commands
    CommitCommands(
        deviceTable.state,
        state->object,
        state->userContext.buffer,
        state->streamState
    );

    // Clear all commands
    state->userContext.buffer.Clear();
}
