// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#pragma once

// Layer
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/SamplerState.h>
#include <Backends/Vulkan/Resource/PushDescriptorSegment.h>
#include <Backends/Vulkan/Resource/DescriptorDataAppendAllocator.h>
#include <Backends/Vulkan/Resource/DescriptorData.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class PushDescriptorAppendAllocator {
public:
    PushDescriptorAppendAllocator(DeviceDispatchTable* table, DescriptorDataAppendAllocator* dataAllocator) : table(table), dataAllocator(dataAllocator) {
        segment.table = table;
    }

    /// Reset this allocator
    void Reset() {
        ASSERT(segment.entries.empty(), "Dangling segment");
        currentLayoutState = nullptr;
    }

    /// Invoked during push descriptor binding
    /// \param commandBuffer object committing from
    /// \param layout the expected pipeline layout
    /// \param set the set index
    /// \param descriptorWriteCount number of writes
    /// \param pDescriptorWrites all writes
    void PushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites) {
        PipelineLayoutState* layoutState = table->states_pipelineLayout.Get(layout);

        // Get the descriptor set layout
        const DescriptorLayoutPhysicalMapping& physicalMapping = layoutState->physicalMapping.descriptorSets.at(set);

        // Ensure sufficient space
        if (set >= setEntries.size()) {
            setEntries.resize(set + 1u);
        }

        // Get the set entry
        SetEntry& setEntry = setEntries.at(set);

        // Is the set out of date?
        if (IsSegmentOutOfDate(layoutState, set)) {
            // Set was out of date, allocate a new segment
            PhysicalResourceSegmentID nextSegmentID = table->prmTable->Allocate(static_cast<uint32_t>(physicalMapping.bindings.size()));

            // If this segment is pending a roll, reconstruct the previous descriptor data
            if (setEntries[set].pendingRoll) {
                // Determine byte offsets
                size_t srcByteOffset = table->prmTable->GetMappingOffset(setEntry.segmentId, 0u) * sizeof(VirtualResourceMapping);
                size_t dstByteOffset = table->prmTable->GetMappingOffset(nextSegmentID, 0u) * sizeof(VirtualResourceMapping);

                // Setup the region
                VkBufferCopy region{};
                region.size = physicalMapping.bindings.size() * sizeof(VirtualResourceMapping);
                region.srcOffset = srcByteOffset;
                region.dstOffset = dstByteOffset;

                // Copy all contents
                table->commandBufferDispatchTable.next_vkCmdCopyBuffer(
                    commandBuffer,
                    table->prmTable->GetDeviceBuffer(),
                    table->prmTable->GetDeviceBuffer(),
                    1u, &region
                );

                // Successfully rolled
                setEntries[set].pendingRoll = false;
            }

            // Don't lose track of the previous segment, kick it to the released segment
            if (setEntry.segmentId != kInvalidPRSID) {
                segment.entries.push_back(setEntry.segmentId);
            }

            // Set new segment id
            setEntry.segmentId = nextSegmentID;
        }

        // Track layout
        currentLayoutState = layoutState;

        // Local stack for mappings
        TrivialStackVector<VirtualResourceMapping, 128> mappingStack;

        // Handle all writes
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& write = pDescriptorWrites[i];

            // Map current binding to an offset
            const uint32_t prmtOffset = physicalMapping.bindings.at(write.dstBinding).prmtOffset;

            // Preallocate (hopefully stack) descriptors
            mappingStack.Resize(write.descriptorCount);
            
            // Create mappings for all descriptors written
            for (uint32_t descriptorIndex = 0; descriptorIndex < write.descriptorCount; descriptorIndex++) {
                // Default invalid mapping
                VirtualResourceMapping& mapping = mappingStack[descriptorIndex];
                mapping.puid = IL::kResourceTokenPUIDMask;

                // Handle type
                switch (write.descriptorType) {
                    default: {
                        // Perhaps handled in the future
                        continue;
                    }
                    case VK_DESCRIPTOR_TYPE_SAMPLER: {
                        if (write.pImageInfo[descriptorIndex].sampler) {
                            mapping = table->states_sampler.Get(write.pImageInfo[descriptorIndex].sampler)->virtualMapping;
                        } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                            mapping.puid = IL::kResourceTokenPUIDReservedNullSampler;
                            mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler);
                            mapping.srb  = 0x1;
                        }
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                        if (write.pImageInfo[descriptorIndex].imageView) {
                            mapping = table->states_imageView.Get(write.pImageInfo[descriptorIndex].imageView)->virtualMapping;
                        } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                            mapping.puid = IL::kResourceTokenPUIDReservedNullTexture;
                            mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
                            mapping.srb  = 0x1;
                        }
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                        if (write.pImageInfo[descriptorIndex].imageView) {
                            mapping = table->states_imageView.Get(write.pImageInfo[descriptorIndex].imageView)->virtualMapping;
                        } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                            mapping.puid = IL::kResourceTokenPUIDReservedNullTexture;
                            mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
                            mapping.srb  = 0x1;
                        }
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
                        if (write.pTexelBufferView[descriptorIndex]) {
                            mapping = table->states_bufferView.Get(write.pTexelBufferView[descriptorIndex])->virtualMapping;
                        } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                            mapping.puid = IL::kResourceTokenPUIDReservedNullBuffer;
                            mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
                            mapping.srb  = 0x1;
                        }
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                        if (write.pBufferInfo[descriptorIndex].buffer) {
                            mapping = table->states_buffer.Get(write.pBufferInfo[descriptorIndex].buffer)->virtualMapping;
                        } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                            mapping.puid = IL::kResourceTokenPUIDReservedNullCBuffer;
                            mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::CBuffer);
                            mapping.srb  = 0x1;
                        }
                        break;
                    }
                }
            }

            // Determine byte offset
            size_t byteOffset = table->prmTable->GetMappingOffset(setEntry.segmentId, prmtOffset + write.dstArrayElement) * sizeof(VirtualResourceMapping);

            // Enqueue copy of stack PRMs
            table->commandBufferDispatchTable.next_vkCmdUpdateBuffer(
                commandBuffer,
                table->prmTable->GetDeviceBuffer(),
                byteOffset,
                mappingStack.Size() * sizeof(VirtualResourceMapping),
                mappingStack.Data()
            );
        }

        // Mark as pending writes
        setEntry.pendingWrite = true;
    }

    /// Invalidate all incompatible slots
    /// \param layoutState state to invalidate against
    void InvalidateOnCompatability(PipelineLayoutState* layoutState) {
        if (!currentLayoutState) {
            return;
        }

        // Invalidate incompatible layouts
        for (size_t i = 0; i < std::min(setEntries.size(), currentLayoutState->compatabilityHashes.size()); i++) {
            if (i >= layoutState->compatabilityHashes.size() ||
                layoutState->compatabilityHashes[i] != currentLayoutState->compatabilityHashes[i]) {
                setEntries[i].segmentId = kInvalidPRSID;
            }
        }

        // Set layout
        currentLayoutState = layoutState;
    }

    /// Commit all changes
    /// \param commandBuffer command buffer to commit to
    /// \param pipelineBindPoint current bind point
    void Commit(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint) {
        bool anyPendingWrite = false;

        // Check all entries
        for (size_t set = 0; set < setEntries.size(); set++) {
            SetEntry& entry = setEntries[set];

            // If a write is pending, descriptor data was enqueued
            if (entry.pendingWrite) {
                entry.pendingWrite = false;

                // Descriptor offsets
                const uint32_t descriptorDataDWordOffset = static_cast<uint32_t>(set * kDescriptorDataDWordCount);
                const uint32_t descriptorDataDWordBound  = currentLayoutState->boundUserDescriptorStates * kDescriptorDataDWordCount;

                // Submit offsets to the data append allocator
                PhysicalResourceMappingTableSegment segmentShader = table->prmTable->GetSegmentShader(entry.segmentId);
                dataAllocator->SetOrAllocate(commandBuffer, descriptorDataDWordOffset + kDescriptorDataLengthDWord, descriptorDataDWordBound, segmentShader.length);
                dataAllocator->Set(commandBuffer, descriptorDataDWordOffset + kDescriptorDataOffsetDWord, segmentShader.offset);

                // Ensure stall on write
                anyPendingWrite = true;
            }

            // Mark as pending roll, the data from here on is considered immutable
            entry.pendingRoll = true;
        }

        // Early out if nothing is to be committed
        if (!anyPendingWrite) {
            return;
        }
        
        // Handle bind point
        VkPipelineStageFlags stageFlags{0};
        switch (pipelineBindPoint) {
            default:
                return;
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                stageFlags = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
                break;
            case VK_PIPELINE_BIND_POINT_COMPUTE:
                stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
        }
        
        // Setup barrier
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = table->prmTable->GetDeviceBuffer();
        barrier.offset = 0x0;
        barrier.size = VK_WHOLE_SIZE;

        // Enqueue barrier
        table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            commandBuffer,
            stageFlags, stageFlags,
            0x0,
            0u, nullptr,
            1u, &barrier,
            0u, nullptr
        );
    }

    /// Release the current segment
    PushDescriptorSegment ReleaseSegment() {
        // Move all pending sets to segment
        for (const SetEntry& setEntry : setEntries) {
            if (setEntry.segmentId != kInvalidPRSID) {
                segment.entries.push_back(setEntry.segmentId);
            }
        }

        // Cleanup local sets
        setEntries.clear();

        // Release ownership
        return std::move(segment);
    }

private:
    /// Check if a segment is out of date
    bool IsSegmentOutOfDate(PipelineLayoutState* layoutState, uint32_t set) {
        // Layout compatability
        if (currentLayoutState && layoutState != currentLayoutState) {
            // Out of date if the previous layout has no such set
            if (set >= currentLayoutState->compatabilityHashes.size()) {
                setEntries[set].pendingRoll = false;
                return true;
            }

            // Out of date if the set layouts are incompatible
            if (currentLayoutState->compatabilityHashes.at(set) != layoutState->compatabilityHashes.at(set)) {
                setEntries[set].pendingRoll = false;
                return true;
            }
        }

        // Out of date if no destination set
        if (setEntries[set].segmentId == kInvalidPRSID) {
            setEntries[set].pendingRoll = false;
            return true;
        }

        // Always out of date if pending a roll
        if (setEntries[set].pendingRoll) {
            return true;
        }

        // OK
        return false;
    }

private:
    struct SetEntry {
        /// Is a previous write pending?
        bool pendingWrite{false};

        /// Is a roll pending? i.e. a new segment is required before a write
        bool pendingRoll{false};

        /// Current segment id
        PhysicalResourceSegmentID segmentId{kInvalidPRSID};
    };
    
private:
    DeviceDispatchTable* table;

    /// The segment to be released
    PushDescriptorSegment segment;

    /// Current layout
    PipelineLayoutState* currentLayoutState{nullptr};

    /// Descriptor data allocator for set writes
    DescriptorDataAppendAllocator* dataAllocator{nullptr};

    /// All set entries
    std::vector<SetEntry> setEntries;
};
