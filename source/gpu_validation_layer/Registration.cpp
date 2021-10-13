#include <cstring>

#include <Common.h>
#include <Callbacks.h>
#include <DispatchTables.h>
#include <CRC.h>
#include <Report.h>

#include <vulkan/vk_layer.h>

namespace Ava {
	static const VkLayerProperties LayerProps[] = {
		{ 
			VK_LAYER_AVA_gpu_validation_NAME,
			VK_LAYER_AVA_gpu_validation_SPEC_VERSION,
			VK_LAYER_AVA_gpu_validation_IMPLEMENTATION_VERSION,
			VK_LAYER_AVA_gpu_validation_DESCRIPTION 
		},
	};

	static const VkExtensionProperties ExtensionProps[] = { { VK_AVA_GPU_VALIDATION_EXTENSION_NAME, VK_AVA_GPU_VALIDATION_SPEC_VERSION } };

	VkResult GetLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
		static constexpr uint32_t NumLayerProps = sizeof(LayerProps) / sizeof(LayerProps[0]);

		if (pProperties == nullptr)
		{
			if (pPropertyCount != nullptr)
			{
				*pPropertyCount = NumLayerProps;
				return VK_SUCCESS;
			}
		}

		if (*pPropertyCount > NumLayerProps)
			*pPropertyCount = NumLayerProps;

		memcpy(pProperties, LayerProps, sizeof(LayerProps[0]) * (*pPropertyCount));
		if (*pPropertyCount != NumLayerProps)
			return VK_INCOMPLETE;
		return VK_SUCCESS;
	}

	VkResult GetExtensionProperties(uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
		static constexpr uint32_t NumExtensionProps = sizeof(ExtensionProps) / sizeof(ExtensionProps[0]);
		if (pProperties == nullptr)
		{
			if (pPropertyCount != nullptr)
			{
				*pPropertyCount = NumExtensionProps;
				return VK_SUCCESS;
			}
		}

		if (*pPropertyCount > NumExtensionProps)
			*pPropertyCount = NumExtensionProps;

		memcpy(pProperties, ExtensionProps, sizeof(ExtensionProps[0]) * (*pPropertyCount));
		if (*pPropertyCount != NumExtensionProps)
			return VK_INCOMPLETE;
		return VK_SUCCESS;
	}

	VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
		return GetLayerProperties(pPropertyCount, pProperties);
	}

	VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
		return GetLayerProperties(pPropertyCount, pProperties);
	}

	VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
		if (pLayerName == nullptr || strcmp(pLayerName, VK_LAYER_AVA_gpu_validation_NAME) != 0) {
			return VK_ERROR_LAYER_NOT_PRESENT;
		}
		return GetExtensionProperties(pPropertyCount, pProperties);
	}

	VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount,
																						VkExtensionProperties* pProperties) {
		// pass through any queries for other layers
		if (pLayerName == nullptr || strcmp(pLayerName, VK_LAYER_AVA_gpu_validation_NAME) != 0) {
			if (physicalDevice == VK_NULL_HANDLE)
				return VK_SUCCESS;

			return InstanceDispatchTable::Get(GetKey(physicalDevice)).m_EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
		}

		return GetExtensionProperties(pPropertyCount, pProperties);
	}

	static VKAPI_ATTR PFN_vkVoidFunction GetSharedProcAddr(void *handle, std::uint64_t crc64) {
		switch (crc64) {
			case "vkCreateDevice"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateDevice);
			case "vkDestroyDevice"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDevice);
            case "vkMapMemory"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&MapMemory);
            case "vkUnmapMemory"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&UnmapMemory);
			case "vkCreatePipelineLayout"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreatePipelineLayout);
			case "vkCreateDescriptorPool"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateDescriptorPool);
			case "vkCreateDescriptorSetLayout"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateDescriptorSetLayout);
			case "vkAllocateDescriptorSets"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&AllocateDescriptorSets);
			case "vkFreeDescriptorSets"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&FreeDescriptorSets);
			case "vkDestroyDescriptorPool"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDescriptorPool);
			case "vkResetDescriptorPool"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&ResetDescriptorPool);
			case "vkUpdateDescriptorSets"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&UpdateDescriptorSets);
			case "vkCreateDescriptorUpdateTemplate"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateDescriptorUpdateTemplate);
			case "vkUpdateDescriptorSetWithTemplate"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&UpdateDescriptorSetWithTemplate);
			case "vkCreateCommandPool"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateCommandPool);
			case "vkCreateImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateImage);
			case "vkCreateImageView"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateImageView);
            case "vkDestroyImage"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&DestroyImage);
            case "vkBindImageMemory"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&BindImageMemory);
            case "vkBindImageMemory2"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&BindImageMemory2);
            case "vkCreateBuffer"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&CreateBuffer);
            case "vkCreateBufferView"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&CreateBufferView);
            case "vkDestroyBuffer"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&DestroyBuffer);
            case "vkBindBufferMemory"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&BindBufferMemory);
            case "vkBindBufferMemory2"_crc64:
                return reinterpret_cast<PFN_vkVoidFunction>(&BindBufferMemory2);
			case "vkCreateRenderPass"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateRenderPass);
			case "vkCreateFramebuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateFramebuffer);
			case "vkAllocateCommandBuffers"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&AllocateCommandBuffers);
			case "vkFreeCommandBuffers"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&FreeCommandBuffers);
			case "vkDestroyPipelineLayout"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyPipelineLayout);
			case "vkDestroyDescriptorSetLayout"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDescriptorSetLayout);
			case "vkDestroyDescriptorUpdateTemplate"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyDescriptorUpdateTemplate);
			case "vkCreateGraphicsPipelines"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateGraphicsPipelines);
			case "vkCreateComputePipelines"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateComputePipelines);
			case "vkBeginCommandBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&BeginCommandBuffer);
			case "vkEndCommandBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&EndCommandBuffer);
			case "vkCmdBindPipeline"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdBindPipeline);
			case "vkDestroyPipeline"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyPipeline);
			case "vkCreateShaderModule"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateShaderModule);
			case "vkDestroyShaderModule"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyShaderModule);
			case "vkCmdBindDescriptorSets"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdBindDescriptorSets);
			case "vkCmdPushConstants"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdPushConstants);
			case "vkCmdPushDescriptorSetKHR"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdPushDescriptorSetKHR);
			case "vkCmdPushDescriptorSetWithTemplateKHR"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdPushDescriptorSetWithTemplateKHR);
			case "vkCmdBeginRenderPass"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdBeginRenderPass);
			case "vkCmdEndRenderPass"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdEndRenderPass);
			case "vkCmdDraw"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDraw);
			case "vkCmdDrawIndexed"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDrawIndexed);
			case "vkCmdDrawIndirect"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDrawIndirect);
			case "vkCmdDrawIndexedIndirect"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDrawIndexedIndirect);
			case "vkCmdDispatch"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDispatch);
			case "vkCmdDispatchIndirect"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdDispatchIndirect);
			case "vkCmdCopyBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdCopyBuffer);
			case "vkCmdCopyImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdCopyImage);
			case "vkCmdBlitImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdBlitImage);
			case "vkCmdCopyBufferToImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdCopyBufferToImage);
			case "vkCmdCopyImageToBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdCopyImageToBuffer);
			case "vkCmdUpdateBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdUpdateBuffer);
			case "vkCmdFillBuffer"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdFillBuffer);
			case "vkCmdClearColorImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdClearColorImage);
			case "vkCmdClearDepthStencilImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdClearDepthStencilImage);
			case "vkCmdClearAttachments"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdClearAttachments);
			case "vkCmdResolveImage"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CmdResolveImage);
			case "vkSetDebugUtilsObjectNameEXT"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&SetDebugUtilsObjectNameEXT);
			case "vkQueueSubmit"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&QueueSubmit);
			case "vkQueuePresentKHR"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&QueuePresentKHR);
			default:
				return nullptr;
		}
	}

	AVA_C_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *pName) {
		std::uint64_t crc64 = ComputeCRC64(pName);
		switch (crc64) {
			case "vkGetDeviceProcAddr"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
			case "vkEnumerateDeviceLayerProperties"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&EnumerateDeviceLayerProperties);
			case "vkEnumerateDeviceExtensionProperties"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&EnumerateDeviceExtensionProperties);
			case "vkGPUValidationCreateReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationCreateReportAVA);
			case "vkGPUValidationDestroyReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationDestroyReportAVA);
			case "vkGPUValidationBeginReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationBeginReportAVA);
			case "vkGPUValidationGetReportStatusAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationGetReportStatusAVA);
			case "vkGPUValidationDrawDebugAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationDrawDebugAVA);
			case "vkGPUValidationEndReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationEndReportAVA);
			case "vkGPUValidationPrintReportSummaryAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationPrintReportSummaryAVA);
			case "vkGPUValidationPrintReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationPrintReportAVA);
			case "vkGPUValidationExportReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationExportReportAVA);
			case "vkGPUValidationGetReportInfoAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationGetReportInfoAVA);
			case "vkGPUValidationFlushReportAVA"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&GPUValidationFlushReportAVA);
		}

		if (PFN_vkVoidFunction addr = GetSharedProcAddr(device, crc64)) {
			return addr;
		}

		return DeviceDispatchTable::Get(GetKey(device))->m_GetDeviceProcAddr(device, pName);
	}

	AVA_C_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
		std::uint64_t crc64 = ComputeCRC64(pName);
		switch (crc64) {
			case "vkGetInstanceProcAddr"_crc64: 
				return reinterpret_cast<PFN_vkVoidFunction>(&vkGetInstanceProcAddr);
			case "vkGetDeviceProcAddr"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&vkGetDeviceProcAddr);
			case "vkEnumerateInstanceLayerProperties"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&EnumerateInstanceLayerProperties);
			case "vkEnumerateInstanceExtensionProperties"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&EnumerateInstanceExtensionProperties);
			case "vkCreateInstance"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&CreateInstance);
			case "vkDestroyInstance"_crc64:
				return reinterpret_cast<PFN_vkVoidFunction>(&DestroyInstance);
		}

		if (PFN_vkVoidFunction addr = GetSharedProcAddr(instance, crc64)) {
			return addr;
		}

		return InstanceDispatchTable::Get(GetKey(instance)).m_GetInstanceProcAddr(instance, pName);
	}

	AVA_C_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
		// Fill in the function pointers if our version is at least capable of having the structure contain them.
		if (pVersionStruct->loaderLayerInterfaceVersion >= 2)
		{
			pVersionStruct->pfnGetInstanceProcAddr		 = vkGetInstanceProcAddr;
			pVersionStruct->pfnGetDeviceProcAddr		 = vkGetDeviceProcAddr;
			pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
		}

		if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION)
		{
			// ...
		}
		else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION)
		{
			pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
		}

		return VK_SUCCESS;
	}
}  // namespace