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
#include <Backends/DX12/Translation.h>
#include <Backends/DX12/RenderPass.h>
#include <Backends/DX12/Table.Gen.h>

// Common
#include "Common/Enum.h"

void CommitCommands(DeviceState* device, ID3D12GraphicsCommandList* commandList, const CommandBuffer& buffer, ShaderExportStreamState* streamState) {
    // Early out if no commands
    if (!buffer.Count()) {
        return;
    }

    // Tracked state
    UserCommandState state;

    // Always end the current render pass if any commands
    if (streamState->renderPass.insideRenderPass) {
        static_cast<ID3D12GraphicsCommandList4*>(commandList)->EndRenderPass();
        state.reconstructionFlags |= ReconstructionFlag::RenderPass;
    }

    // Check if copy
    const bool isCopyCommandList = commandList->GetType() == D3D12_COMMAND_LIST_TYPE_COPY;

    // Default clearing chunk size
    static constexpr size_t kClearChunkSize = static_cast<size_t>(8e6);

    // Shared clear chunk, allocated on demand
    ShaderExportConstantAllocation sharedClearChunk;
    
    // Handle all commands
    for (const Command &command: buffer) {
        switch (static_cast<CommandType>(command.commandType)) {
            default: {
                ASSERT(false, "Invalid command for target");
                break;
            }
            case CommandType::SetShaderProgram: {
                auto *cmd = command.As<SetShaderProgramCommand>();

                // Update state
                state.reconstructionFlags |= ReconstructionFlag::Pipeline;
                state.shaderProgramID = cmd->id;

                // Set pipeline
                commandList->SetComputeRootSignature(device->shaderProgramHost->GetSignature(cmd->id));
                commandList->SetPipelineState(device->shaderProgramHost->GetPipeline(cmd->id));

                // Get the number of bindings
                uint32_t bindingCount = 0;
                device->shaderDataHost->EnumerateProgram(state.shaderProgramID, &bindingCount, nullptr, ShaderDataType::BindingMask);

                // If there's no local bindings, bind the *current* shader export
                if (!bindingCount) {
                    device->exportStreamer->BindShaderExport(streamState, 0u, PipelineType::Compute, commandList);
                }
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
            case CommandType::SetResource: {
                auto *cmd = command.As<SetResourceCommand>();

                // Get the root index
                uint32_t bindingIndex = device->shaderDataHost->GetBindingIndex(state.shaderProgramID, cmd->id);

                // Lazy allocate
                if (bindingIndex >= state.shaderProgramBindings.Size()) {
                    state.shaderProgramBindings.Resize(bindingIndex + 1);
                }

                ResourceState *resourceState = device->physicalResourceIdentifierMap.GetState(cmd->puid);
                
                // Set binding
                state.shaderProgramBindings[bindingIndex] = UserBinding {
                    .resource = resourceState->object,
                    .width = resourceState->desc.Width
                };
                break;
            }
            case CommandType::SetResourceData: {
                auto *cmd = command.As<SetResourceDataCommand>();

                // Get the root index
                uint32_t bindingIndex = device->shaderDataHost->GetBindingIndex(state.shaderProgramID, cmd->id);

                // Lazy allocate
                if (bindingIndex >= state.shaderProgramBindings.Size()) {
                    state.shaderProgramBindings.Resize(bindingIndex + 1);
                }

                const Allocation &allocation = device->shaderDataHost->GetResourceAllocation(cmd->buffer);

                // Set binding
                state.shaderProgramBindings[bindingIndex] = UserBinding {
                    .resource = allocation.resource,
                    .width = allocation.resource->GetDesc().Width
                };
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

                // Shader Write -> Copy Dest
                if (!isCopyCommandList) {
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Transition.pResource = allocation.resource;
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                    commandList->ResourceBarrier(1u, &barrier);
                }

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

                // Copy Dest -> Shader Write
                if (!isCopyCommandList) {
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Transition.pResource = allocation.resource;
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    commandList->ResourceBarrier(1u, &barrier);
                }
                break;
            }
            case CommandType::ClearBuffer: {
                auto* cmd = command.As<ClearBufferCommand>();

                // The reason we're not using typical clear commands is because
                // copy queues don't support them. So, we instead copy from a zeroed
                // resource. TODO[init]: Could add a non-copy queue path! But would
                // pollute the descriptor heap, unless you swap them around.

                // Allocate shared chunk if needed
                if (!sharedClearChunk.resource) {
                    sharedClearChunk = streamState->constantAllocator.Allocate(device->deviceAllocator, kClearChunkSize);
                    std::memset(sharedClearChunk.staging, 0x0u, kClearChunkSize);
                }

                // Clearing to specific values isn't supported yet
                // Would be trivial, just need to do it
                ASSERT(cmd->value == 0x0, "Unsupported clear value");

                // Number of writes
                size_t chunkCount = (cmd->length + kClearChunkSize - 1) / kClearChunkSize;

                // Get the data allocation
                Allocation allocation = device->shaderDataHost->GetResourceAllocation(cmd->id);

                // Write all chunks
                for (size_t i = 0; i < chunkCount; i++) {
                    size_t offset = kClearChunkSize * i;
                    ASSERT(cmd->length > offset, "Invalid offset");
                    
                    size_t length = std::min(kClearChunkSize, cmd->length - offset);

                    // Copy from chunk to resource
                    commandList->CopyBufferRegion(
                        allocation.resource,
                        cmd->offset + offset,
                        sharedClearChunk.resource,
                        sharedClearChunk.offset,
                        length
                    );
                }
                break;
            }
            case CommandType::CopyBuffer: {
                auto* cmd = command.As<CopyBufferCommand>();

                // Get the data allocations
                Allocation source = device->shaderDataHost->GetResourceAllocation(cmd->source);
                Allocation dest = device->shaderDataHost->GetResourceAllocation(cmd->dest);
                
                // Copy resource
                commandList->CopyBufferRegion(
                    dest.resource,
                    cmd->destOffset,
                    source.resource,
                    cmd->sourceOffset,
                    cmd->byteCount
                );
                break;
            }
            case CommandType::Discard: {
                auto* cmd = command.As<DiscardCommand>();

                // Get the resource state
                ResourceState* resourceState = device->physicalResourceIdentifierMap.GetState(cmd->puid);
                ASSERT(resourceState, "Invalid resource PUID");

                // Discard the entire resource
                commandList->DiscardResource(resourceState->object, nullptr);
                break;
            }
            case CommandType::BeginPredicate: {
                auto* cmd = command.As<BeginPredicateCommand>();

                const Allocation& predicateBuffer = device->shaderDataHost->GetResourceAllocation(cmd->buffer);

                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = predicateBuffer.resource;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PREDICATION;
                commandList->ResourceBarrier(1u, &barrier);
                    
                commandList->SetPredication(
                    predicateBuffer.resource, cmd->offset,
                    D3D12_PREDICATION_OP_EQUAL_ZERO
                );
                break;
            }
            case CommandType::EndPredicate: {
                auto* cmd = command.As<EndPredicateCommand>();

                const Allocation& predicateBuffer = device->shaderDataHost->GetResourceAllocation(cmd->buffer);
                
                commandList->SetPredication(
                    nullptr, 0,
                    D3D12_PREDICATION_OP_EQUAL_ZERO
                );
                    
                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = predicateBuffer.resource;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PREDICATION;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                commandList->ResourceBarrier(1u, &barrier);
                break;
            }
            case CommandType::Dispatch:
            case CommandType::DispatchIndirect: {
                // Any resources to set?
                if (state.shaderProgramBindings.Size()) {
                    // Update state
                    state.reconstructionFlags |= ReconstructionFlag::Heap;

                    // Number of bindings
                    uint32_t bindingCount = 0;
                    device->shaderDataHost->EnumerateProgram(state.shaderProgramID, &bindingCount, nullptr, ShaderDataType::BindingMask);

                    // Get all program bindings
                    std::vector<ShaderDataInfo> bindings(bindingCount);
                    device->shaderDataHost->EnumerateProgram(state.shaderProgramID, &bindingCount, bindings.data(), ShaderDataType::BindingMask);

                    // We're expecting them all to be bound
                    ASSERT(bindingCount == state.shaderProgramBindings.Size(), "Unexpected binding count");

                    // Allocate both the user bindings + shader export
                    ShaderExportOwnedHeapAllocation heapAllocation = streamState->heapAllocator.Allocate(
                        device,
                        bindingCount + device->exportStreamer->GetShaderExportDescriptorCount()
                    );

                    // Switch to the shared heap
                    commandList->SetDescriptorHeaps(1u, &heapAllocation.heap);

                    // Create all descriptors
                    for (size_t i = 0; i < bindingCount; i++) {
                        UserBinding &binding = state.shaderProgramBindings[i];

                        // Shader wise data info
                        const ShaderDataInfo& dataInfo = bindings[i];

                        // UAV or SRV?
                        if (dataInfo.bufferBinding.isWritable) {
                            D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
                            view.Format = Translate(dataInfo.bufferBinding.format);
                            view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                            view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                            view.Buffer.FirstElement = 0;
                            view.Buffer.NumElements = static_cast<UINT>(binding.width / Backend::IL::GetSize(dataInfo.bufferBinding.format));
                
                            // Create descriptor
                            device->object->CreateUnorderedAccessView(
                                binding.resource, nullptr,
                                &view,
                                heapAllocation.CPU(static_cast<uint32_t>(i))
                            );
                        } else {
                            D3D12_SHADER_RESOURCE_VIEW_DESC view{};
                            view.Format = Translate(dataInfo.bufferBinding.format);
                            view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                            view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                            view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                            view.Buffer.FirstElement = 0;
                            view.Buffer.StructureByteStride = 0;
                            view.Buffer.NumElements = static_cast<UINT>(binding.width / Backend::IL::GetSize(dataInfo.bufferBinding.format));
                
                            // Create descriptor
                            device->object->CreateShaderResourceView(
                                binding.resource,
                                &view,
                                heapAllocation.CPU(static_cast<uint32_t>(i))
                            );
                        }
                    }

                    // Create the shader export handle in the shared allocation
                    device->exportStreamer->CreateExternalShaderExport(streamState, heapAllocation.Advance(bindingCount));

                    // Bind tables
                    commandList->SetComputeRootDescriptorTable(0, heapAllocation.GPU(0));
                    commandList->SetComputeRootDescriptorTable(1, heapAllocation.GPU(bindingCount));

                    // Clear last bindings
                    state.shaderProgramBindings.Clear();
                }

                // Invoke
                if (static_cast<CommandType>(command.commandType) == CommandType::DispatchIndirect) {
                    auto* cmd = command.As<DispatchIndirectCommand>();

                    const Allocation& indirectBuffer = device->shaderDataHost->GetResourceAllocation(cmd->buffer);

                    // UAV -> Indirect
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Transition.pResource = indirectBuffer.resource;
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                    commandList->ResourceBarrier(1u, &barrier);
                    
                    // Execute with shared signature
                    commandList->ExecuteIndirect(
                        device->shaderProgramHost->GetIndirectCommandSignature(),
                        1u,
                        indirectBuffer.resource, cmd->offset,
                        nullptr, 0
                    );
                    
                    // Indirect -> UAV
                    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    commandList->ResourceBarrier(1u, &barrier);
                } else {
                    auto* cmd = command.As<DispatchCommand>();

                    commandList->Dispatch(
                        cmd->groupCountX,
                        cmd->groupCountY,
                        cmd->groupCountZ
                    );
                }
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

    // Cleanup
    state.shaderProgramBindings.Clear();
    
    // Reconstruct user state
    ReconstructState(device, commandList, streamState, state.reconstructionFlags);
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
