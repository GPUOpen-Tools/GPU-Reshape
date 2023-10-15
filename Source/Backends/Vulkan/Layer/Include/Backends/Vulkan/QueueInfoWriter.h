// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Common
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Containers/LinearBlockAllocator.h>

// Layer
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

/// Helper for exclusive queue allocation
class QueueInfoWriter {
public:
    QueueInfoWriter(DeviceDispatchTable* table) : table(table) {
        // Copy all queue data
        entries.Resize(table->createInfo->queueCreateInfoCount);
        std::memcpy(entries.Data(), table->createInfo->pQueueCreateInfos, table->createInfo->queueCreateInfoCount * sizeof(VkDeviceQueueCreateInfo));
    }

    /// \brief Get an exclusive queue of type, shares if none are available
    /// \param flags Required flags, attempts to find the nearest match
    /// \return The exclusive queue
    ExclusiveQueue RequestExclusiveQueueOfType(VkQueueFlags flags) {
        ExclusiveQueue queue;

        // Find the most appropriate family
        queue.familyIndex = FindQueueFamilyWithFlags(flags);

        // May not be supported
        if (queue.familyIndex == UINT32_MAX) {
            return queue;
        }

        // Get family
        const VkQueueFamilyProperties& family = table->queueFamilyProperties[queue.familyIndex];

        // Check all existing queues
        for (VkDeviceQueueCreateInfo& createInfo : entries) {
            if (createInfo.queueFamilyIndex != queue.familyIndex) {
                continue;
            }

            // Exceeded queue count?
            // Just use the last one
            if (createInfo.queueCount == family.queueCount) {
                queue.queueIndex = createInfo.queueCount - 1u;
                return queue;
            }

            // Queues are available, increment
            queue.queueIndex = createInfo.queueCount;
            createInfo.queueCount++;

            // OK
            return queue;
        }

        // No match was found, append a new request
        VkDeviceQueueCreateInfo& entry = entries.Add({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
        entry.queueFamilyIndex = queue.familyIndex;
        entry.queueCount = 1;
        queue.queueIndex = 0;

        // OK
        return queue;
    }

    /// Assign all queues
    void Assign() {
        // Consolidate all data
        Consolidate();

        // Set new queues
        table->createInfo->pQueueCreateInfos = entries.Data();
        table->createInfo->queueCreateInfoCount = static_cast<uint32_t>(entries.Size());
    }

private:
    /// Consolidate all indirections
    void Consolidate() {
        for (uint32_t i = 0; i < entries.Size(); i++) {
            VkDeviceQueueCreateInfo& entry = entries[i];

            // User queue count, zero if a new queue
            uint32_t userCount = i < table->createInfo->queueCreateInfoCount ? table->createInfo->pQueueCreateInfos[i].queueCount : 0u;

            // Copy all old priorities
            auto* priorities = allocator.AllocateArray<float>(entry.queueCount);
            std::memcpy(priorities, entry.pQueuePriorities, userCount * sizeof(float));

            // Fill new priorities, default to 1.0f
            std::fill_n(priorities + userCount, entry.queueCount - userCount, 1.0f);

            // Assign priorities
            entry.pQueuePriorities = priorities;
        }
    }

    /// \brief Find a queue family with a set of flags
    /// \param flags Required flags, attempts to find the nearest match
    /// \return Queue index
    uint32_t FindQueueFamilyWithFlags(VkQueueFlags flags) {
        uint32_t candidate = UINT32_MAX;
        uint32_t distance  = UINT32_MAX;

        // Visit all families
        for (size_t i = 0; i < table->queueFamilyProperties.size(); i++) {
            const VkQueueFamilyProperties& properties = table->queueFamilyProperties[i];

            // Contains requested flags?
            if ((properties.queueFlags & flags) != flags) {
                continue;
            }

            // Better match?
            if (properties.queueFlags < distance) {
                candidate = static_cast<uint32_t>(i);
                distance = properties.queueFlags;
            }
        }

        // OK
        return candidate;
    }

private:
    // All queue creation requests
    TrivialStackVector<VkDeviceQueueCreateInfo, 16> entries;
    
    /// Internal allocator
    LinearBlockAllocator<4096> allocator;

private:
    // Target table
    DeviceDispatchTable* table;
};
