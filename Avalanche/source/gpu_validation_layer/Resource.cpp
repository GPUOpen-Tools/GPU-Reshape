#include <Callbacks.h>
#include <StateTables.h>
#include <Pipeline.h>
#include <set>
#include <CRC.h>
#include <cstring>
#include <algorithm>

// Specialized passes
#include <Private/Passes/DataResidency/ResourceInitializationPass.h>

template<typename F>
VkResult FSEmulate(VkDevice device, F&& functor)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Must be serial
    std::lock_guard<std::mutex> guard(device_state->m_FSLock);

    VkResult result;
    for (auto&& kv : device_state->m_FSQueues)
    {
        SPendingQueueInitialization& pqi = kv.second;

        // Do not propagate commands if it is not destined for submission
        if (pqi.m_MissedFrameCounter > kPQIMissedFrameThreshold)
            continue;

        // Needs submission assignment?
        if (!pqi.m_CurrentSubmission.m_CommandBuffer)
        {
            // Attempt to circulate current pending list
            for (auto it = pqi.m_PendingSubmissions.begin(); it != pqi.m_PendingSubmissions.end(); it++)
            {
                if (table->m_GetFenceStatus(device, it->m_Fence) != VK_SUCCESS)
                    continue;

                // Reset for next submission
                table->m_ResetFences(device, 1, &it->m_Fence);

                // Pop
                pqi.m_CurrentSubmission = *it;
                pqi.m_PendingSubmissions.erase(it);
				break;
            }

            // Needs a new one?
            if (!pqi.m_CurrentSubmission.m_CommandBuffer)
            {
                // Initialization command buffer
                VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                alloc_info.commandBufferCount = 1;
                alloc_info.commandPool = pqi.m_Pool;
                alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

                // Allocate temporary command buffer
                if ((result = table->m_AllocateCommandBuffers(device, &alloc_info, &pqi.m_CurrentSubmission.m_CommandBuffer)) != VK_SUCCESS)
                {
                    return result;
                }

                // Attempt to create fence
                VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                if ((result = table->m_CreateFence(device, &fence_info, nullptr, &pqi.m_CurrentSubmission.m_Fence)) != VK_SUCCESS)
                {
                    return result;
                }

                // Patch the internal dispatch tables
                InstanceDispatchTable instance_table = InstanceDispatchTable::Get(GetKey(table->m_Instance));
                PatchDispatchTable(instance_table, device, pqi.m_CurrentSubmission.m_CommandBuffer);
                PatchDispatchTable(instance_table, device, pqi.m_CurrentSubmission.m_Fence);
            }

            // Begin
            VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            if ((result = table->m_CmdBeginCommandBuffer(pqi.m_CurrentSubmission.m_CommandBuffer, &begin_info)) != VK_SUCCESS)
            {
                return result;
            }
        }

        functor(pqi.m_CurrentSubmission.m_CommandBuffer);
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_MapMemory(device, memory, offset, size, flags, ppData);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

        // Mark as mapped
        // Resources may be created post-mapped
        STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[memory];
        mem.m_IsMapped = true;

        // Resource initialization pass?
        if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
        {
            // Enumlate initialization of contained resources
            FSEmulate(device, [pass, &mem](VkCommandBuffer cmd_buffer)
            {
                for (VkBuffer buffer : mem.m_Buffers)
                {
                    pass->InitializeBuffer(cmd_buffer, buffer);
                }

                for (VkImage image : mem.m_Images)
                {
                    VkImageSubresourceRange range{};
                    range.layerCount = VK_REMAINING_ARRAY_LAYERS;
                    range.levelCount = VK_REMAINING_MIP_LEVELS;
                    pass->InitializeImage(cmd_buffer, image, range);
                }
            });
        }
    }

    return VK_SUCCESS;
}

void VKAPI_CALL UnmapMemory(VkDevice device, VkDeviceMemory memory)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    table->m_UnmapMemory(device, memory);

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

        // Mark as unmapped
        STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[memory];
        mem.m_IsMapped = false;
    }
}

VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Pass down callchain
	VkResult result = table->m_CreateImage(device, pCreateInfo, pAllocator, pImage);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track source
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		device_state->m_ResourceImageSources[*pImage] = *pCreateInfo;
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Pass down callchain
	VkResult result = table->m_CreateImageView(device, pCreateInfo, pAllocator, pView);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track source
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		device_state->m_ResourceImageViewSources[*pView] = *pCreateInfo;
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    return VK_SUCCESS;
}

VkResult CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Track source
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
        device_state->m_ResourceBufferViewSources[*pView] = *pCreateInfo;
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_BindBufferMemory(device, buffer, memory, memoryOffset);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
        STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[memory];

        // Track memory source
        device_state->m_ResourceBufferMemory[buffer] = memory;

        // Append this buffer
        mem.m_Buffers.push_back(buffer);

        // Already mapped?
        if (mem.m_IsMapped)
        {
            // Resource initialization pass?
            if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
            {
                FSEmulate(device, [pass, buffer](VkCommandBuffer cmd_buffer)
                {
                    pass->InitializeBuffer(cmd_buffer, buffer);
                });
            }
        }
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_BindImageMemory(device, image, memory, memoryOffset);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
        STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[memory];

        // Track memory source
        device_state->m_ResourceImageMemory[image] = memory;

        // Append this imkage
        mem.m_Images.push_back(image);

        // Already mapped?
        if (mem.m_IsMapped)
        {
            // Resource initialization pass?
            if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
            {
                FSEmulate(device, [pass, image](VkCommandBuffer cmd_buffer)
                {
                    VkImageSubresourceRange range{};
                    range.layerCount = VK_REMAINING_ARRAY_LAYERS;
                    range.levelCount = VK_REMAINING_MIP_LEVELS;
                    pass->InitializeImage(cmd_buffer, image, range);
                });
            }
        }
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL BindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_BindBufferMemory2(device, bindInfoCount, pBindInfos);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

        for (uint32_t i = 0; i < bindInfoCount; i++)
        {
            auto&& bind = pBindInfos[i];

            STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[bind.memory];

            // Track memory source
            device_state->m_ResourceBufferMemory[bind.buffer] = bind.memory;

            // Append this buffer
            mem.m_Buffers.push_back(bind.buffer);

            // Already mapped?
            if (mem.m_IsMapped)
            {
                // Resource initialization pass?
                if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
                {
                    FSEmulate(device, [pass, bind](VkCommandBuffer cmd_buffer)
                    {
                        pass->InitializeBuffer(cmd_buffer, bind.buffer);
                    });
                }
            }
        }
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL BindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // Pass down callchain
    VkResult result = table->m_BindImageMemory2(device, bindInfoCount, pBindInfos);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Resource tracking
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

        for (uint32_t i = 0; i < bindInfoCount; i++)
        {
            auto&& bind = pBindInfos[i];

            STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[bind.memory];

            // Track memory source
            device_state->m_ResourceImageMemory[bind.image] = bind.memory;

            // Append this image
            mem.m_Images.push_back(bind.image);

            // Already mapped?
            if (mem.m_IsMapped)
            {
                // Resource initialization pass?
                if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
                {
                    FSEmulate(device, [pass, bind](VkCommandBuffer cmd_buffer)
                    {
                        VkImageSubresourceRange range{};
                        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
                        range.levelCount = VK_REMAINING_MIP_LEVELS;
                        pass->InitializeImage(cmd_buffer, bind.image, range);
                    });
                }
            }
        }
    }

    return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Pass down callchain
	VkResult result = table->m_CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track source
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		if (pCreateInfo->pSubpasses[0].pDepthStencilAttachment)
		{
			device_state->m_ResourceRenderPassDepthSlots[*pRenderPass] = pCreateInfo->pSubpasses[0].pDepthStencilAttachment->attachment;
		}
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFramebuffer * pFramebuffer)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Pass down callchain
	VkResult result = table->m_CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track source
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		
		// Add all views
		auto&& views = device_state->m_ResourceFramebufferSources[*pFramebuffer];
		for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++)
		{
			views.push_back(pCreateInfo->pAttachments[i]);
		}
	}

	return VK_SUCCESS;
}

void VKAPI_CALL DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // The RI pass must be serial
    std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

    // Resource initialization pass?
    if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
    {
        FSEmulate(device, [pass, buffer](VkCommandBuffer cmd_buffer)
        {
            pass->FreeBuffer(cmd_buffer, buffer);
        });
    }

    // Pass down callchain
    table->m_DestroyBuffer(device, buffer, pAllocator);

    // Resource tracking
    {
        // May not have been bound
        auto mem_it = device_state->m_ResourceBufferMemory.find(buffer);
        if (mem_it != device_state->m_ResourceBufferMemory.end())
        {
            STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[(*mem_it).second];

            // Remove by value
            mem.m_Buffers.erase(std::remove(mem.m_Buffers.begin(), mem.m_Buffers.end(), buffer), mem.m_Buffers.end());
        }
    }
}

void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

    // The RI pass must be serial
    std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

    // Resource initialization pass?
    if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
    {
        FSEmulate(device, [pass, image](VkCommandBuffer cmd_buffer)
        {
            pass->FreeImage(cmd_buffer, image);
        });
    }

    // Pass down callchain
    table->m_DestroyImage(device, image, pAllocator);

    // Resource tracking
    {
        // May not have been bound
        auto mem_it = device_state->m_ResourceImageMemory.find(image);
        if (mem_it != device_state->m_ResourceImageMemory.end())
        {
            STrackedDeviceMemory &mem = device_state->m_ResourceDeviceMemory[(*mem_it).second];

            // Remove by value
            mem.m_Images.erase(std::remove(mem.m_Images.begin(), mem.m_Images.end(), image), mem.m_Images.end());
        }
    }
}