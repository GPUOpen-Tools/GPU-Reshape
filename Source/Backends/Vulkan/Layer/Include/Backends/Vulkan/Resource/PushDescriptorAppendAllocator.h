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
#include <Backends/Vulkan/States/DescriptorUpdateTemplateState.h>
#include <Backends/Vulkan/Resource/PushDescriptorSegment.h>
#include <Backends/Vulkan/Resource/DescriptorDataAppendAllocator.h>
#include <Backends/Vulkan/Resource/DescriptorData.h>
#include <Backends/Vulkan/Resource/DescriptorResourceMapping.h>

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

        // Get the set entry
        SetEntry& setEntry = GetEntryFor(layoutState, set, commandBuffer);

        // Handle all writes
        for (uint32_t i = 0; i < descriptorWriteCount; i++) {
            const VkWriteDescriptorSet& write = pDescriptorWrites[i];

            // Map current binding to an offset
            const uint32_t prmtOffset = physicalMapping.bindings.at(write.dstBinding).prmtOffset;

            // Create mappings for all descriptors written
            for (uint32_t descriptorIndex = 0; descriptorIndex < write.descriptorCount; descriptorIndex++) {
                // Write mapping
                table->prmTable->WriteMapping(setEntry.segmentId, prmtOffset + write.dstArrayElement, GetVirtualResourceMapping(table, write, descriptorIndex));
            }
        }

        // Mark as pending writes
        setEntry.pendingWrite = true;
    }

    /// Invoked during push descriptor binding
    /// \param commandBuffer  object committing from
    /// \param descriptorUpdateTemplate the update template
    /// \param layout the expected pipeline layout
    /// \param set the set index
    /// \param pData update data
    void PushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, DescriptorUpdateTemplateState* descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) {
        PipelineLayoutState* layoutState = table->states_pipelineLayout.Get(layout);

        // Get the descriptor set layout
        const DescriptorLayoutPhysicalMapping& physicalMapping = layoutState->physicalMapping.descriptorSets.at(set);

        // Get the set entry
        SetEntry& setEntry = GetEntryFor(layoutState, set, commandBuffer);
        
        // Handle each entry
        for (uint32_t i = 0; i < descriptorUpdateTemplate->createInfo->descriptorUpdateEntryCount; i++) {
            const VkDescriptorUpdateTemplateEntry& entry = descriptorUpdateTemplate->createInfo->pDescriptorUpdateEntries[i];

            // Map current binding to an offset
            const uint32_t prmtOffset = physicalMapping.bindings.at(entry.dstBinding).prmtOffset;

            // Handle each binding write
            for (uint32_t descriptorIndex = 0; descriptorIndex < entry.descriptorCount; descriptorIndex++) {
                const void *descriptorData = static_cast<const uint8_t*>(pData) + entry.offset + descriptorIndex * entry.stride;
                
                // Write mapping
                table->prmTable->WriteMapping(setEntry.segmentId, prmtOffset + entry.dstArrayElement, GetVirtualResourceMapping(table, entry.descriptorType, descriptorData));
            }
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
            }

            // Mark as pending roll, the data from here on is considered immutable
            entry.pendingRoll = true;
        }
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

    /// Get the entry for a specific set
    SetEntry& GetEntryFor(PipelineLayoutState* layoutState, uint32_t set, VkCommandBuffer commandBuffer) {
        // Ensure sufficient space
        if (set >= setEntries.size()) {
            setEntries.resize(set + 1u);
        }

        // Get the descriptor set layout
        const DescriptorLayoutPhysicalMapping& physicalMapping = layoutState->physicalMapping.descriptorSets.at(set);
        
        // Get the set entry
        SetEntry& setEntry = setEntries.at(set);

        // Is the set out of date?
        if (IsSegmentOutOfDate(layoutState, set)) {
            // Set was out of date, allocate a new segment
            PhysicalResourceSegmentID nextSegmentID = table->prmTable->Allocate(static_cast<uint32_t>(physicalMapping.bindings.size()));

            // If this segment is pending a roll, reconstruct the previous descriptor data
            if (setEntries[set].pendingRoll) {
                // Copy all mappings
                table->prmTable->CopyMappings(setEntry.segmentId, nextSegmentID);
                
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

        // OK
        return setEntry;
    }
    
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
