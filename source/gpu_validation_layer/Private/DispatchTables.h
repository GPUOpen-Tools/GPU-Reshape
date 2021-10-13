#pragma once

#include "Common.h"

#include <map>
#include <mutex>
#include <vector>
#include <Common.h>

/**
 * Get the vulkan object dispatch table key
 */
template<typename T>
inline void*& GetKey(T inst)
{
	return *(void**)inst;
}

/**
 * Patch the internal dispatch tables
 * ? The validation layers are a bit shite sometimes
 */
template<typename T, typename U>
inline void ForcePatchDispatchTable(T source, U dest)
{
	GetKey(dest) = GetKey(source);
}

// Represents all callable instance dispatches
struct InstanceDispatchTable
{
	/**
	 * Add a new entry
	 * @param[in] key the key of the entry
	 * @param[in] table the dispatch table
	 */
	static void Add(void* key, const InstanceDispatchTable& table)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table[key] = table;
	}

	/**
	 * Remove an existing entry
	 * @param[in] key the key of the entry
	 */
	static void Remove(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table.erase(key);
	}

	/**
	 * Get an existing entry
	 * @param[in] key the key of the entry
	 */
	static InstanceDispatchTable Get(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return Table[key];
	}

	VkInstance m_Instance;
	bool       m_RequiresDispatchTablePatching;

	PFN_vkGetInstanceProcAddr					 m_GetInstanceProcAddr;
	PFN_vkDestroyInstance						 m_DestroyInstance;
	PFN_vkEnumerateDeviceExtensionProperties	 m_EnumerateDeviceExtensionProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties m_GetPhysicalDeviceQueueFamilyProperties;

private:
	static std::mutex							  Mutex;
	static std::map<void*, InstanceDispatchTable> Table;
};

// Represents all callable device dispatches
struct DeviceDispatchTable
{
	/**
	 * Add a new entry
	 * @param[in] key the key of the entry
	 * @param[in] table the dispatch table
	 */
	static void Add(void* key, DeviceDispatchTable* table)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table[key] = table;
	}

	/**
	 * Remove an existing entry
	 * @param[in] key the key of the entry
	 */
	static void Remove(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table.erase(key);
	}

	/**
	 * Get an existing entry
	 * @param[in] key the key of the entry
	 */
	static DeviceDispatchTable* Get(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return Table[key];
	}

	VkDevice					 m_Device;
	VkInstance					 m_Instance;
	VkPhysicalDevice			 m_PhysicalDevice;
	VkGPUValidationCreateInfoAVA m_CreateInfoAVA;

	std::vector<VkQueueFamilyProperties> m_QueueFamilies;
	VkDeviceQueueCreateInfo				 m_DedicatedTransferQueueInfo;
	VkDeviceQueueCreateInfo				 m_SharedGraphicsQueueInfo;
	VkDeviceQueueCreateInfo				 m_DedicatedCopyEmulationQueueInfo;

	PFN_vkGetDeviceProcAddr					  m_GetDeviceProcAddr;
	PFN_vkCreatePipelineLayout				  m_CreatePipelineLayout;
	PFN_vkDestroyPipelineLayout				  m_DestroyPipelineLayout;
	PFN_vkCreateGraphicsPipelines			  m_CreateGraphicsPipelines;
	PFN_vkCreateComputePipelines			  m_CreateComputePipelines;
	PFN_vkDestroyDevice						  m_DestroyDevice;
	PFN_vkDestroyBuffer						  m_DestroyBuffer;
    PFN_vkCreateImage						  m_CreateImage;
    PFN_vkDestroyImage						  m_DestroyImage;
	PFN_vkCreateImageView					  m_CreateImageView;
	PFN_vkCreateRenderPass					  m_CreateRenderPass;
	PFN_vkCreateFramebuffer					  m_CreateFramebuffer;
	PFN_vkDestroyBufferView					  m_DestroyBufferView;
	PFN_vkDestroyDescriptorPool				  m_DestroyDescriptorPool;
	PFN_vkDestroyDescriptorUpdateTemplate     m_DestroyDescriptorUpdateTemplate;
	PFN_vkResetDescriptorPool				  m_ResetDescriptorPool;
	PFN_vkFreeDescriptorSets				  m_FreeDescriptorSet;
	PFN_vkDestroyDescriptorSetLayout		  m_DestroyDescriptorSetLayout;
	PFN_vkDestroyPipeline					  m_DestroyPipeline;
	PFN_vkDestroyCommandPool				  m_DestroyCommandPool;
	PFN_vkDestroySemaphore					  m_DestroySemaphore;
	PFN_vkDestroyFence						  m_DestroyFence;
	PFN_vkCreateShaderModule				  m_CreateShaderModule;
	PFN_vkDestroyShaderModule				  m_DestroyShaderModule;
	PFN_vkCmdBindPipeline					  m_CmdBindPipeline;
	PFN_vkResetCommandBuffer				  m_CmdResetCommandBuffer;
	PFN_vkCmdFillBuffer						  m_CmdFillBuffer;
	PFN_vkBeginCommandBuffer				  m_CmdBeginCommandBuffer;
	PFN_vkEndCommandBuffer					  m_CmdEndCommandBuffer;
	PFN_vkAllocateMemory					  m_AllocateMemory;
	PFN_vkFreeMemory						  m_FreeMemory;
	PFN_vkCreateDescriptorPool				  m_CreateDescriptorPool;
	PFN_vkCreateDescriptorSetLayout			  m_CreateDescriptorSetLayout;
	PFN_vkCreateDescriptorUpdateTemplate	  m_CreateDescriptorUpdateTemplate;
	PFN_vkAllocateDescriptorSets			  m_AllocateDescriptorSets;
	PFN_vkGetPhysicalDeviceMemoryProperties2  m_GetPhysicalDeviceMemoryProperties2;
	PFN_vkGetPhysicalDeviceProperties2		  m_GetPhysicalDeviceProperties2;
	PFN_vkCreateBuffer						  m_CreateBuffer;
	PFN_vkCreateBufferView					  m_CreateBufferView;
	PFN_vkUpdateDescriptorSets				  m_UpdateDescriptorSets;
	PFN_vkUpdateDescriptorSetWithTemplate	  m_UpdateDescriptorSetWithTemplate;
	PFN_vkCreateEvent						  m_CreateEvent;
	PFN_vkCreateFence						  m_CreateFence;
	PFN_vkCmdSetEvent						  m_CmdSetEvent;
	PFN_vkSetEvent							  m_SetEvent;
	PFN_vkResetEvent						  m_ResetEvent;
	PFN_vkResetFences						  m_ResetFences;
    PFN_vkBindBufferMemory					  m_BindBufferMemory;
    PFN_vkBindImageMemory					  m_BindImageMemory;
    PFN_vkBindBufferMemory2					  m_BindBufferMemory2;
    PFN_vkBindImageMemory2					  m_BindImageMemory2;
	PFN_vkCmdBindDescriptorSets				  m_BindDescriptorSets;
	PFN_vkGetBufferMemoryRequirements		  m_GetBufferMemoryRequirements;
	PFN_vkGetEventStatus					  m_GetEventStatus;
	PFN_vkGetFenceStatus					  m_GetFenceStatus;
	PFN_vkFlushMappedMemoryRanges			  m_FlushMappedMemoryRanges;
	PFN_vkInvalidateMappedMemoryRanges		  m_InvalidateMappedMemoryRanges;
	PFN_vkMapMemory							  m_MapMemory;
	PFN_vkUnmapMemory						  m_UnmapMemory;
	PFN_vkQueuePresentKHR					  m_QueuePresentKHR;
	PFN_vkCmdPushConstants					  m_CmdPushConstants;
	PFN_vkCmdPushDescriptorSetKHR			  m_CmdPushDescriptorSetKHR;
	PFN_vkCmdPushDescriptorSetWithTemplateKHR m_CmdPushDescriptorSetWithTemplateKHR;
    PFN_vkSetDebugUtilsObjectNameEXT		  m_SetDebugUtilsObjectNameEXT;
	PFN_vkCmdUpdateBuffer					  m_CmdUpdateBuffer;
	PFN_vkCmdPipelineBarrier				  m_CmdPipelineBarrier;
	PFN_vkCmdCopyBuffer						  m_CmdCopyBuffer;
	PFN_vkCmdCopyImage						  m_CmdCopyImage;
	PFN_vkCmdBlitImage						  m_CmdBlitImage;
	PFN_vkCmdCopyBufferToImage				  m_CmdCopyBufferToImage;
	PFN_vkCmdCopyImageToBuffer				  m_CmdCopyImageToBuffer;
	PFN_vkCreateSemaphore					  m_CreateSemaphore;
	PFN_vkCreateCommandPool					  m_CreateCommandPool;
	PFN_vkAllocateCommandBuffers			  m_AllocateCommandBuffers;
	PFN_vkFreeCommandBuffers				  m_FreeCommandBuffers;
	PFN_vkGetDeviceQueue					  m_GetDeviceQueue;
	PFN_vkCmdBeginRenderPass				  m_CmdBeginRenderPass;
	PFN_vkCmdEndRenderPass					  m_CmdEndRenderPass;
	PFN_vkCmdDraw							  m_CmdDraw;
	PFN_vkCmdDrawIndexed					  m_CmdDrawIndexed;
	PFN_vkCmdDrawIndirect					  m_CmdDrawIndirect;
	PFN_vkCmdDrawIndexedIndirect		      m_CmdDrawIndexedIndirect;
	PFN_vkCmdDispatch						  m_CmdDispatch;
	PFN_vkCmdDispatchIndirect				  m_CmdDispatchIndirect;
	PFN_vkCmdClearColorImage				  m_CmdClearColorImage;
	PFN_vkCmdClearDepthStencilImage			  m_CmdClearDepthStencilImage;
	PFN_vkCmdClearAttachments				  m_CmdClearAttachments;
	PFN_vkCmdResolveImage					  m_CmdResolveImage;
	PFN_vkQueueSubmit						  m_QueueSubmit;
	PFN_vkDeviceWaitIdle					  m_DeviceWaitIdle;
	PFN_vkQueueWaitIdle						  m_QueueWaitIdle;

private:
	static std::mutex							 Mutex;
	static std::map<void*, DeviceDispatchTable*> Table;
};

/**
 * Patch the internal dispatch tables
 * ? The validation layers are a bit shite sometimes
 */
template<typename T, typename U>
inline void PatchDispatchTable(InstanceDispatchTable& table, T source, U dest)
{
	if (table.m_RequiresDispatchTablePatching)
	{
		ForcePatchDispatchTable(source, dest);
	}
}
