#include <Callbacks.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <DiagnosticRegistry.h>
#include <cstring>
#include <sys/stat.h>
#include <ShaderCompiler.h>
#include <PipelineCompiler.h>
#include <ShaderCache.h>
#include <DiagnosticAllocator.h>

/* Passes */
#include <Passes/StateVersionBreadcrumbPass.h>
#include <Passes/Basic/ResourceBoundsPass.h>
#include <Passes/Basic/ExportStabilityPass.h>
#include <Passes/Basic/RuntimeArrayBoundsPass.h>
#include <Passes/Concurrency/ResourceDataRacePass.h>
#include <Passes/DataResidency/ResourceInitializationPass.h>

/* Vulkan */
#include <spirv-tools/libspirv.h>
#include <spirv-tools/optimizer.hpp>

VkResult CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
	InstanceDispatchTable instance_table = InstanceDispatchTable::Get(GetKey(physicalDevice));

	auto table = new DeviceDispatchTable{};

	// Attempt to find ava info
	if (auto ava_info = FindStructureType<VkGPUValidationCreateInfoAVA>(pCreateInfo, VK_STRUCTURE_TYPE_GPU_VALIDATION_CREATE_INFO_AVA))
	{
		table->m_CreateInfoAVA = *ava_info;

		if (ava_info->m_CommandBufferMessageCountDefault == 0)
		{
			if (ava_info->m_LogCallback && (ava_info->m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				ava_info->m_LogCallback(ava_info->m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "CommandBufferMessageCountDefault must be greather than 0");
			}

			return VK_ERROR_INITIALIZATION_FAILED;
		}

		if (ava_info->m_CommandBufferMessageCountLimit < ava_info->m_CommandBufferMessageCountDefault)
		{
			if (ava_info->m_LogCallback && (ava_info->m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				ava_info->m_LogCallback(ava_info->m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "CommandBufferMessageCountLimit must be greather or equal than CommandBufferMessageCountDefault");
			}

			return VK_ERROR_INITIALIZATION_FAILED;
		}
	}
	else
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Attempt to find link info
	auto chain_info = (VkLayerDeviceCreateInfo*)pCreateInfo->pNext;
	while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO))
	{
		chain_info = (VkLayerDeviceCreateInfo*)chain_info->pNext;
	}

	// ...
	if (!chain_info)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// Fetch previous addresses
	PFN_vkGetInstanceProcAddr getInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
	PFN_vkGetDeviceProcAddr   getDeviceProcAddr   = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;

	// Advance layer
	chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

	// Copy queues
	std::vector<VkDeviceQueueCreateInfo> queues(pCreateInfo->queueCreateInfoCount);
	std::memcpy(queues.data(), pCreateInfo->pQueueCreateInfos, sizeof(VkDeviceQueueCreateInfo) * queues.size());

	// Append transfer queue if possible
	{
		uint32_t family_count;
		instance_table.m_GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &family_count, nullptr);

		table->m_QueueFamilies.resize(family_count);
		instance_table.m_GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &family_count, table->m_QueueFamilies.data());
		
		// Search for dedicated transfer
		size_t shared_graphics_index = 0;
		for (size_t i = 0; i < table->m_QueueFamilies.size(); i++)
		{
			const VkQueueFamilyProperties& family = table->m_QueueFamilies[i];

			// Find graphics queue, guarenteed
			if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !table->m_SharedGraphicsQueueInfo.queueCount)
			{
				for (const VkDeviceQueueCreateInfo& info : queues)
				{
					if (info.queueFamilyIndex == i)
					{
						shared_graphics_index = i;
						table->m_SharedGraphicsQueueInfo = info;
						break;
					}
				}
			}

			// Find dedicated compute queue for copy emulation
			if ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(family.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !table->m_DedicatedCopyEmulationQueueInfo.queueCount)
			{
				for (const VkDeviceQueueCreateInfo& info : queues)
				{
					if (info.queueFamilyIndex == i)
					{
						table->m_DedicatedCopyEmulationQueueInfo = info;
						break;
					}
				}
			}

			// Only care about dedicated transfer queues
			if (!(family.queueFlags & VK_QUEUE_TRANSFER_BIT) || (family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
				continue;

			// The user may already have supplied the above index for usage
			for (VkDeviceQueueCreateInfo& info : queues)
			{
				if (info.queueFamilyIndex == i)
				{
					// Schedule work on secondary queue within the same family to avoid scheduling contentions
					info.queueCount++;
					table->m_DedicatedTransferQueueInfo = info;
					break;
				}
			}

			// Append if not
			if (!table->m_DedicatedTransferQueueInfo.queueCount)
			{
				VkDeviceQueueCreateInfo info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				info.queueCount = 1;
				info.queueFamilyIndex = static_cast<uint32_t>(i);
				queues.push_back(info);
			}
		}

		// No dedicated emulation?
		if (!table->m_DedicatedCopyEmulationQueueInfo.queueCount)
		{
			table->m_DedicatedCopyEmulationQueueInfo = table->m_SharedGraphicsQueueInfo;

			// Schedule work on secondary queue within the same family to avoid scheduling contentions
			queues[shared_graphics_index].queueCount++;
		}
	}

	// Prepare creation info
	VkDeviceCreateInfo create_info = *pCreateInfo;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
	create_info.pQueueCreateInfos = queues.data();

#if 0 /* Reserved for future usage */
	// Enable the memory model feature
	// Required for in-queue-family atomic operations
	VkPhysicalDeviceVulkanMemoryModelFeaturesKHR memory_model_khr{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES_KHR };
	memory_model_khr.pNext = const_cast<void*>(pCreateInfo->pNext); // The sdk is inconsistent
	memory_model_khr.vulkanMemoryModelDeviceScope = true;
	memory_model_khr.vulkanMemoryModel = true;
	create_info.pNext = &memory_model_khr;

	// Copy top extensions
	auto extensions = ALLOCA_ARRAY(const char*, create_info.enabledExtensionCount + 1);
	std::memcpy(extensions, create_info.ppEnabledExtensionNames, sizeof(const char*) * create_info.enabledExtensionCount);
	{
		extensions[create_info.enabledExtensionCount++] = VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME;
		create_info.ppEnabledExtensionNames = extensions;
	}
#endif

	// Pass down the chain
	VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, &create_info, pAllocator, pDevice);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Populate dispatch table
	table->m_Device = *pDevice;
	table->m_Instance = instance_table.m_Instance;
	table->m_PhysicalDevice = physicalDevice;
	table->m_GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)getDeviceProcAddr(*pDevice, "vkGetDeviceProcAddr");
	table->m_CreatePipelineLayout = (PFN_vkCreatePipelineLayout)getDeviceProcAddr(*pDevice, "vkCreatePipelineLayout");
	table->m_DestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)getDeviceProcAddr(*pDevice, "vkDestroyPipelineLayout");
	table->m_DestroyDevice = (PFN_vkDestroyDevice)getDeviceProcAddr(*pDevice, "vkDestroyDevice");
	table->m_DestroyBuffer = (PFN_vkDestroyBuffer)getDeviceProcAddr(*pDevice, "vkDestroyBuffer");
	table->m_DestroyBufferView = (PFN_vkDestroyBufferView)getDeviceProcAddr(*pDevice, "vkDestroyBufferView");
	table->m_DestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)getDeviceProcAddr(*pDevice, "vkDestroyDescriptorPool");
	table->m_DestroyCommandPool = (PFN_vkDestroyCommandPool)getDeviceProcAddr(*pDevice, "vkDestroyCommandPool");
	table->m_ResetDescriptorPool = (PFN_vkResetDescriptorPool)getDeviceProcAddr(*pDevice, "vkResetDescriptorPool");
	table->m_FreeDescriptorSet = (PFN_vkFreeDescriptorSets)getDeviceProcAddr(*pDevice, "vkFreeDescriptorSets");
	table->m_DestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)getDeviceProcAddr(*pDevice, "vkDestroyDescriptorSetLayout");
	table->m_DestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)getDeviceProcAddr(*pDevice, "vkDestroyDescriptorUpdateTemplate");
	table->m_CreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)getDeviceProcAddr(*pDevice, "vkCreateGraphicsPipelines");
	table->m_CreateComputePipelines = (PFN_vkCreateComputePipelines)getDeviceProcAddr(*pDevice, "vkCreateComputePipelines");
	table->m_DestroySemaphore = (PFN_vkDestroySemaphore)getDeviceProcAddr(*pDevice, "vkDestroySemaphore");
	table->m_DestroyFence = (PFN_vkDestroyFence)getDeviceProcAddr(*pDevice, "vkDestroyFence");
	table->m_DestroyPipeline = (PFN_vkDestroyPipeline)getDeviceProcAddr(*pDevice, "vkDestroyPipeline");
	table->m_CmdBindPipeline = (PFN_vkCmdBindPipeline)getDeviceProcAddr(*pDevice, "vkCmdBindPipeline");
	table->m_CreateShaderModule = (PFN_vkCreateShaderModule)getDeviceProcAddr(*pDevice, "vkCreateShaderModule");
	table->m_DestroyShaderModule = (PFN_vkDestroyShaderModule)getDeviceProcAddr(*pDevice, "vkDestroyShaderModule");
	table->m_CmdBeginCommandBuffer = (PFN_vkBeginCommandBuffer)getDeviceProcAddr(*pDevice, "vkBeginCommandBuffer");
	table->m_CmdEndCommandBuffer = (PFN_vkEndCommandBuffer)getDeviceProcAddr(*pDevice, "vkEndCommandBuffer");
	table->m_AllocateMemory = (PFN_vkAllocateMemory)getDeviceProcAddr(*pDevice, "vkAllocateMemory");
	table->m_FreeMemory = (PFN_vkFreeMemory)getDeviceProcAddr(*pDevice, "vkFreeMemory");
	table->m_CreateDescriptorPool = (PFN_vkCreateDescriptorPool)getDeviceProcAddr(*pDevice, "vkCreateDescriptorPool");
	table->m_CreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)getDeviceProcAddr(*pDevice, "vkCreateDescriptorSetLayout");
	table->m_CreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)getDeviceProcAddr(*pDevice, "vkCreateDescriptorUpdateTemplate");
	table->m_GetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)getInstanceProcAddr(instance_table.m_Instance, "vkGetPhysicalDeviceMemoryProperties2");
	table->m_GetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)getInstanceProcAddr(instance_table.m_Instance, "vkGetPhysicalDeviceProperties2");
    table->m_CreateImage = (PFN_vkCreateImage)getDeviceProcAddr(*pDevice, "vkCreateImage");
    table->m_DestroyImage = (PFN_vkDestroyImage)getDeviceProcAddr(*pDevice, "vkDestroyImage");
    table->m_CreateImageView = (PFN_vkCreateImageView)getDeviceProcAddr(*pDevice, "vkCreateImageView");
	table->m_CreateRenderPass = (PFN_vkCreateRenderPass)getDeviceProcAddr(*pDevice, "vkCreateRenderPass");
	table->m_CreateFramebuffer = (PFN_vkCreateFramebuffer)getDeviceProcAddr(*pDevice, "vkCreateFramebuffer");
	table->m_CreateBuffer = (PFN_vkCreateBuffer)getDeviceProcAddr(*pDevice, "vkCreateBuffer");
	table->m_CreateBufferView = (PFN_vkCreateBufferView)getDeviceProcAddr(*pDevice, "vkCreateBufferView");
	table->m_AllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)getDeviceProcAddr(*pDevice, "vkAllocateDescriptorSets");
	table->m_UpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)getDeviceProcAddr(*pDevice, "vkUpdateDescriptorSets");
	table->m_UpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)getDeviceProcAddr(*pDevice, "vkUpdateDescriptorSetWithTemplate");
	table->m_CreateEvent = (PFN_vkCreateEvent)getDeviceProcAddr(*pDevice, "vkCreateEvent");
	table->m_CreateFence = (PFN_vkCreateFence)getDeviceProcAddr(*pDevice, "vkCreateFence");
    table->m_BindBufferMemory = (PFN_vkBindBufferMemory)getDeviceProcAddr(*pDevice, "vkBindBufferMemory");
    table->m_BindImageMemory = (PFN_vkBindImageMemory)getDeviceProcAddr(*pDevice, "vkBindImageMemory");
    table->m_BindBufferMemory2 = (PFN_vkBindBufferMemory2)getDeviceProcAddr(*pDevice, "vkBindBufferMemory2");
    table->m_BindImageMemory2 = (PFN_vkBindImageMemory2)getDeviceProcAddr(*pDevice, "vkBindImageMemory2");
	table->m_BindDescriptorSets = (PFN_vkCmdBindDescriptorSets)getDeviceProcAddr(*pDevice, "vkCmdBindDescriptorSets");
	table->m_GetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)getDeviceProcAddr(*pDevice, "vkGetBufferMemoryRequirements");
	table->m_GetEventStatus = (PFN_vkGetEventStatus)getDeviceProcAddr(*pDevice, "vkGetEventStatus");
	table->m_FlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)getDeviceProcAddr(*pDevice, "vkFlushMappedMemoryRanges");
	table->m_InvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)getDeviceProcAddr(*pDevice, "vkInvalidateMappedMemoryRanges");
	table->m_MapMemory = (PFN_vkMapMemory)getDeviceProcAddr(*pDevice, "vkMapMemory");
	table->m_UnmapMemory = (PFN_vkUnmapMemory)getDeviceProcAddr(*pDevice, "vkUnmapMemory");
	table->m_CmdPushConstants = (PFN_vkCmdPushConstants)getDeviceProcAddr(*pDevice, "vkCmdPushConstants");
	table->m_CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)getDeviceProcAddr(*pDevice, "vkCmdPushDescriptorSetKHR");
	table->m_CmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)getDeviceProcAddr(*pDevice, "vkCmdPushDescriptorSetWithTemplateKHR");
	table->m_SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)getDeviceProcAddr(*pDevice, "vkSetDebugUtilsObjectNameEXT");
	table->m_QueueSubmit = (PFN_vkQueueSubmit)getDeviceProcAddr(*pDevice, "vkQueueSubmit");
	table->m_CmdSetEvent = (PFN_vkCmdSetEvent)getDeviceProcAddr(*pDevice, "vkCmdSetEvent");
	table->m_SetEvent = (PFN_vkSetEvent)getDeviceProcAddr(*pDevice, "vkSetEvent");
	table->m_ResetEvent = (PFN_vkResetEvent)getDeviceProcAddr(*pDevice, "vkResetEvent");
	table->m_ResetFences = (PFN_vkResetFences)getDeviceProcAddr(*pDevice, "vkResetFences");
	table->m_GetFenceStatus = (PFN_vkGetFenceStatus)getDeviceProcAddr(*pDevice, "vkGetFenceStatus");
	table->m_QueuePresentKHR = (PFN_vkQueuePresentKHR)getDeviceProcAddr(*pDevice, "vkQueuePresentKHR");
	table->m_CmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)getDeviceProcAddr(*pDevice, "vkCmdUpdateBuffer");
	table->m_CmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)getDeviceProcAddr(*pDevice, "vkCmdPipelineBarrier");
	table->m_CmdCopyBuffer = (PFN_vkCmdCopyBuffer)getDeviceProcAddr(*pDevice, "vkCmdCopyBuffer");
	table->m_CmdCopyImage = (PFN_vkCmdCopyImage)getDeviceProcAddr(*pDevice, "vkCmdCopyImage");
	table->m_CmdBlitImage = (PFN_vkCmdBlitImage)getDeviceProcAddr(*pDevice, "vkCmdBlitImage");
	table->m_CmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)getDeviceProcAddr(*pDevice, "vkCmdCopyBufferToImage");
	table->m_CmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)getDeviceProcAddr(*pDevice, "vkCmdCopyImageToBuffer");
	table->m_CreateSemaphore = (PFN_vkCreateSemaphore)getDeviceProcAddr(*pDevice, "vkCreateSemaphore");
	table->m_CreateCommandPool = (PFN_vkCreateCommandPool)getDeviceProcAddr(*pDevice, "vkCreateCommandPool");
	table->m_AllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)getDeviceProcAddr(*pDevice, "vkAllocateCommandBuffers");
	table->m_FreeCommandBuffers = (PFN_vkFreeCommandBuffers)getDeviceProcAddr(*pDevice, "vkFreeCommandBuffers");
	table->m_GetDeviceQueue = (PFN_vkGetDeviceQueue)getDeviceProcAddr(*pDevice, "vkGetDeviceQueue");
	table->m_CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)getDeviceProcAddr(*pDevice, "vkCmdBeginRenderPass");
	table->m_CmdEndRenderPass = (PFN_vkCmdEndRenderPass)getDeviceProcAddr(*pDevice, "vkCmdEndRenderPass");
	table->m_CmdDraw = (PFN_vkCmdDraw)getDeviceProcAddr(*pDevice, "vkCmdDraw");
	table->m_CmdDrawIndexed = (PFN_vkCmdDrawIndexed)getDeviceProcAddr(*pDevice, "vkCmdDrawIndexed");
	table->m_CmdDrawIndirect = (PFN_vkCmdDrawIndirect)getDeviceProcAddr(*pDevice, "vkCmdDrawIndirect");
	table->m_CmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)getDeviceProcAddr(*pDevice, "vkCmdDrawIndexedIndirect");
	table->m_CmdDispatch = (PFN_vkCmdDispatch)getDeviceProcAddr(*pDevice, "vkCmdDispatch");
	table->m_CmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)getDeviceProcAddr(*pDevice, "vkCmdDispatchIndirect");
	table->m_CmdResetCommandBuffer = (PFN_vkResetCommandBuffer)getDeviceProcAddr(*pDevice, "vkResetCommandBuffer");
	table->m_CmdFillBuffer = (PFN_vkCmdFillBuffer)getDeviceProcAddr(*pDevice, "vkCmdFillBuffer");
	table->m_CmdClearColorImage = (PFN_vkCmdClearColorImage)getDeviceProcAddr(*pDevice, "vkCmdClearColorImage");
	table->m_CmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)getDeviceProcAddr(*pDevice, "vkCmdClearDepthStencilImage");
	table->m_CmdClearAttachments = (PFN_vkCmdClearAttachments)getDeviceProcAddr(*pDevice, "vkCmdClearAttachments");
	table->m_CmdResolveImage = (PFN_vkCmdResolveImage)getDeviceProcAddr(*pDevice, "vkCmdResolveImage");
	table->m_DeviceWaitIdle = (PFN_vkDeviceWaitIdle)getDeviceProcAddr(*pDevice, "vkDeviceWaitIdle");
	table->m_QueueWaitIdle = (PFN_vkQueueWaitIdle)getDeviceProcAddr(*pDevice, "vkQueueWaitIdle");
	DeviceDispatchTable::Add(GetKey(*pDevice), table);

	// Create state
	{
		auto state = new DeviceStateTable();
		DeviceStateTable::Add(GetKey(*pDevice), state);

		// Initialize the registry
        state->m_DiagnosticRegistry = std::make_unique<DiagnosticRegistry>();
		state->m_DiagnosticRegistry->Initialize(table->m_CreateInfoAVA);

		// Get dedicated transfer queue
		if (table->m_DedicatedTransferQueueInfo.queueCount)
		{
			// Create async transfer queue if requested
			if (table->m_CreateInfoAVA.m_AsyncTransfer)
			{
				// Note: Secondary queue within the family
				table->m_GetDeviceQueue(*pDevice, table->m_DedicatedTransferQueueInfo.queueFamilyIndex, table->m_DedicatedTransferQueueInfo.queueCount - 1, &state->m_TransferQueue);

				// Patch the internal dispatch table
				PatchDispatchTable(instance_table, table->m_Device, state->m_TransferQueue);

				// Create dedicated pool
				VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
				pool_info.queueFamilyIndex = table->m_DedicatedTransferQueueInfo.queueFamilyIndex;
				pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				if ((result = table->m_CreateCommandPool(*pDevice, &pool_info, pAllocator, &state->m_TransferPool)) != VK_SUCCESS)
				{
					return result;
				}

				// Dummy blob
				for (size_t i = 0; i < table->m_QueueFamilies.size(); i++)
				{
					state->m_QueueFamilyIndices.push_back(static_cast<uint32_t>(i));
				}

				// ...
				state->m_DedicatedTransferQueueFamily = table->m_DedicatedTransferQueueInfo.queueFamilyIndex;
			}

			// Create first-submission emulation queues
			for (uint32_t qci_index = 0; qci_index < create_info.queueCreateInfoCount; qci_index++)
            {
			    // Must have compute capabilities
			    if (!(table->m_QueueFamilies[create_info.pQueueCreateInfos[qci_index].queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT))
			        continue;

			    for (uint32_t q_index = 0; q_index < create_info.pQueueCreateInfos[qci_index].queueCount; q_index++)
                {
			        // Get the underlying queue
			        VkQueue queue;
                    table->m_GetDeviceQueue(*pDevice, create_info.pQueueCreateInfos[qci_index].queueFamilyIndex, q_index, &queue);

                    // Prepare state
                    SPendingQueueInitialization& pqi = state->m_FSQueues[queue];
                    pqi.m_MissedFrameCounter = 0;

                    // Create dedicated pool
                    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                    pool_info.queueFamilyIndex = create_info.pQueueCreateInfos[qci_index].queueFamilyIndex;
                    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                    if ((result = table->m_CreateCommandPool(*pDevice, &pool_info, pAllocator, &pqi.m_Pool)) != VK_SUCCESS)
                    {
                        return result;
                    }
                }
            }

			// Create copy emulation queue
			{
				// Get the emulated queue
				table->m_GetDeviceQueue(*pDevice, table->m_DedicatedTransferQueueInfo.queueFamilyIndex, 0, &state->m_EmulatedTransferQueue);

				// Note: Secondary queue within the family
				table->m_GetDeviceQueue(*pDevice, table->m_DedicatedCopyEmulationQueueInfo.queueFamilyIndex, table->m_DedicatedCopyEmulationQueueInfo.queueCount - 1, &state->m_CopyEmulationQueue);

				// Patch the internal dispatch table
				PatchDispatchTable(instance_table, table->m_Device, state->m_CopyEmulationQueue);

				// Create dedicated pool
				VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
				pool_info.queueFamilyIndex = table->m_DedicatedCopyEmulationQueueInfo.queueFamilyIndex;
				pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				if ((result = table->m_CreateCommandPool(*pDevice, &pool_info, pAllocator, &state->m_CopyEmulationPool)) != VK_SUCCESS)
				{
					return result;
				}

				// ...
				state->m_DedicatedCopyEmulationQueueFamily = table->m_DedicatedCopyEmulationQueueInfo.queueFamilyIndex;
			}
		}

		// Create shader cache
		state->m_ShaderCache = std::make_unique<ShaderCache>();

		// Cache is optional
		if (table->m_CreateInfoAVA.m_CacheFilePath)
		{
			state->m_ShaderCache->Initialize(*pDevice);

			// TODO: Configurable auto serialization?
			state->m_ShaderCache->SetAutoSerialization(table->m_CreateInfoAVA.m_CacheFilePath, 10, 2.f);

			// Deserialize existing cache if present
			struct stat buffer;
			if (stat(table->m_CreateInfoAVA.m_CacheFilePath, &buffer) == 0)
			{
				state->m_ShaderCache->Deserialize(table->m_CreateInfoAVA.m_CacheFilePath);
			}
		}

		// Create compilers
        state->m_ShaderCompiler = std::make_unique<ShaderCompiler>();
        state->m_PipelineCompiler = std::make_unique<PipelineCompiler>();

		// Initialize compilers
		state->m_ShaderCompiler->Initialize(*pDevice, table->m_CreateInfoAVA.m_ShaderCompilerWorkerCount);
		state->m_PipelineCompiler->Initialize(*pDevice, table->m_CreateInfoAVA.m_PipelineCompilerWorkerCount);

		// Get properties
		table->m_GetPhysicalDeviceProperties2(table->m_PhysicalDevice, &(state->m_PhysicalDeviceProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }));

		// Initialize registry
		{
		    // Breadcrumbs
            state->m_DiagnosticRegistry->Register(Passes::kBreadcrumbPassID, new Passes::StateVersionBreadcrumbPass(table, state));

            // Basic Instrumentation Set
			state->m_DiagnosticRegistry->Register(VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS, new Passes::Basic::ResourceBoundsPass(table, state));
			state->m_DiagnosticRegistry->Register(VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY, new Passes::Basic::ExportStabilityPass(table, state));
			state->m_DiagnosticRegistry->Register(VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS, new Passes::Basic::RuntimeArrayBoundsPass(table, state));

			// Concurrency Instrumentation Set
			state->m_DiagnosticRegistry->Register(VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE, new Passes::Concurrency::ResourceDataRacePass(table, state));

			// Data Residency Instrumentation Set
			state->m_DiagnosticRegistry->Register(VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION, new Passes::DataResidency::ResourceInitializationPass(table, state));
		}

		// Initialize allocator
		state->m_DiagnosticAllocator = std::make_unique<DiagnosticAllocator>();
		state->m_DiagnosticAllocator->Initialize(instance_table.m_Instance, table->m_PhysicalDevice, *pDevice, pAllocator, state->m_DiagnosticRegistry.get());
		{
			state->m_DiagnosticAllocator->SetThrottleThreshold(table->m_CreateInfoAVA.m_ThrottleThresholdDefault);
		}

		{
			// Get graphics queue
			VkQueue graphics_queue;
			table->m_GetDeviceQueue(*pDevice, table->m_SharedGraphicsQueueInfo.queueFamilyIndex, 0, &graphics_queue);

			// Patch the internal dispatch table
			PatchDispatchTable(instance_table, table->m_Device, graphics_queue);

			// Initialization pool
			VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			pool_info.queueFamilyIndex = table->m_SharedGraphicsQueueInfo.queueFamilyIndex;
			pool_info.flags = 0;

			// Create temporary pool
			VkCommandPool pool;
			if ((result = table->m_CreateCommandPool(*pDevice, &pool_info, pAllocator, &pool)) != VK_SUCCESS)
			{
				return result;
			}

			// Patch the internal dispatch table
			PatchDispatchTable(instance_table, *pDevice, pool);

			// Initialization command buffer
			VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			alloc_info.commandBufferCount = 1;
			alloc_info.commandPool = pool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			// Allocate temporary command buffer
			VkCommandBuffer cmd_buffer;
			if ((result = table->m_AllocateCommandBuffers(*pDevice, &alloc_info, &cmd_buffer)) != VK_SUCCESS)
			{
				return result;
			}

			// Patch the internal dispatch table
			PatchDispatchTable(instance_table, *pDevice, cmd_buffer);

			// Record all initialization commands
			{
				VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				if ((result = table->m_CmdBeginCommandBuffer(cmd_buffer, &begin_info)) != VK_SUCCESS)
				{
					return result;
				}

				// Post initialize all passes
				state->m_DiagnosticRegistry->InitializePasses(cmd_buffer);

				if ((result = table->m_CmdEndCommandBuffer(cmd_buffer)) != VK_SUCCESS)
				{
					return result;
				}
			}

			// Submit on primary queue and wait
			VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &cmd_buffer;
			table->m_QueueSubmit(graphics_queue, 1, &submit_info, nullptr);
			table->m_QueueWaitIdle(graphics_queue);

			// Destroy initialization states
			// Note: The validation layers REALLY don't like me destroying it here, I have no clue why
			// table->m_DestroyCommandPool(*pDevice, pool, pAllocator);
		}
	}

	// OK
	return VK_SUCCESS;
}

VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
	return EnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
	if (!pLayerName || std::strcmp(pLayerName, "VK_LAYER_AVA_GPU_VALIDATION") != 0)
	{
		if (!physicalDevice)
		{
			return VK_SUCCESS;
		}

		// Pass down the chain
		return InstanceDispatchTable::Get(GetKey(physicalDevice)).m_EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
	}

	if (pPropertyCount)
		*pPropertyCount = 0;
	return VK_SUCCESS;
}

void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Close FS emulation pipes
	for (auto&& kv : state->m_FSQueues)
    {
	    SPendingQueueInitialization& pqi = kv.second;
	    if (pqi.m_CurrentSubmission.m_CommandBuffer)
        {
	        table->m_CmdEndCommandBuffer(pqi.m_CurrentSubmission.m_CommandBuffer);
        }
    }

	// Cleanup state
	{
		// Release systems (in order)
		state->m_DiagnosticRegistry->Release();
		state->m_DiagnosticAllocator->Release();
		state->m_PipelineCompiler->Release();
		state->m_ShaderCompiler->Release();
		state->m_ShaderCache->Release();

		// Free global transfer pool
		table->m_DestroyCommandPool(device, state->m_TransferPool, nullptr);
	}
	delete state;

	// Note that the key lookup is unsafe after device destruction
	void* device_key = GetKey(device);

	// Pass down call chain
	table->m_DestroyDevice(device, pAllocator);

	// Remove lookups
	DeviceDispatchTable::Remove(device_key);
	DeviceStateTable::Remove(device_key);

	// Cleanup table
	delete table;
}