#pragma once

#include "Common.h"
#include <cstring>

namespace Detail
{
	template<typename T>
	T* ByteCopy(size_t* size, uint8_t*& out, const T& value)
	{
		if (!out)
		{
			*size += sizeof(T);
			return nullptr;
		}

		auto dest = out;
		std::memcpy(dest, &value, sizeof(T));
		out += sizeof(T);
		return reinterpret_cast<T*>(dest);
	}

	template<typename T>
	T* ByteCopyOptional(size_t* size, uint8_t*& out, const T* value)
	{
		if (!value)
			return nullptr;

		return ByteCopy(size, out, *value);
	}

	template<typename T, typename U = typename std::remove_const<T>::type>
	U* ByteCopyArray(size_t* size, uint8_t*& out, T* array, uint32_t count)
	{
		if (!out)
		{
			*size += sizeof(T) * count;
			return nullptr;
		}

		auto dest = out;
		std::memcpy(dest, array, sizeof(T) * count);
		out += sizeof(T) * count;
		return reinterpret_cast<U*>(dest);
	}

	template<typename T, typename U = typename std::remove_const<T>::type>
	U* ByteCopyOptionalArray(size_t* size, uint8_t*& out, T* array, uint32_t count)
	{
		if (!array)
			return nullptr;

		return ByteCopyArray(size, out, array, count);
	}

	VkGraphicsPipelineCreateInfo* DeepCopy(size_t* size, uint8_t*& out, const VkGraphicsPipelineCreateInfo& info)
	{
		auto mirror = ByteCopy(size, out, info);
		
		auto stages = ByteCopyArray(size, out, info.pStages, info.stageCount);

		auto vertexInputState = ByteCopyOptional(size, out, info.pVertexInputState);
		auto inputAssemblyState = ByteCopyOptional(size, out, info.pInputAssemblyState);
		auto tessellationState = ByteCopyOptional(size, out, info.pTessellationState);
		auto viewportState = ByteCopyOptional(size, out, info.pViewportState);
		auto rasterizationState = ByteCopyOptional(size, out, info.pRasterizationState);
		auto multisampleState = ByteCopyOptional(size, out, info.pMultisampleState);
		auto depthStencilState = ByteCopyOptional(size, out, info.pDepthStencilState);
		auto colorBlendState = ByteCopyOptional(size, out, info.pColorBlendState);
		auto dynamicState = ByteCopyOptional(size, out, info.pDynamicState);

		VkVertexInputBindingDescription* vertexBindingDescriptions = nullptr;
		VkVertexInputAttributeDescription* vertexAttributeDescriptions = nullptr;
		if (info.pVertexInputState)
		{
			vertexBindingDescriptions = ByteCopyOptionalArray(size, out, info.pVertexInputState->pVertexBindingDescriptions, info.pVertexInputState->vertexBindingDescriptionCount);
			vertexAttributeDescriptions = ByteCopyOptionalArray(size, out, info.pVertexInputState->pVertexAttributeDescriptions, info.pVertexInputState->vertexAttributeDescriptionCount);
		}

		VkViewport* viewports = nullptr;
		VkRect2D* viewportScissors = nullptr;
		if (info.pViewportState)
		{
			viewports = ByteCopyOptionalArray(size, out, info.pViewportState->pViewports, info.pViewportState->viewportCount);
			viewportScissors = ByteCopyOptionalArray(size, out, info.pViewportState->pScissors, info.pViewportState->scissorCount);
		}

		VkSampleMask* sampleMask = ByteCopyOptional(size, out, info.pMultisampleState->pSampleMask);

		VkPipelineColorBlendAttachmentState* colorBlendAttachments = nullptr;
		if (info.pColorBlendState)
		{
			colorBlendAttachments = ByteCopyOptionalArray(size, out, info.pColorBlendState->pAttachments, info.pColorBlendState->attachmentCount);
		}

		VkDynamicState* dynamicStates = nullptr;
		if (info.pDynamicState)
		{
			dynamicStates = ByteCopyOptionalArray(size, out, info.pDynamicState->pDynamicStates, info.pDynamicState->dynamicStateCount);
		}

		if (mirror)
		{
			mirror->pStages = stages;
			mirror->pVertexInputState = vertexInputState;
			mirror->pInputAssemblyState = inputAssemblyState;
			mirror->pTessellationState = tessellationState;
			mirror->pViewportState = viewportState;
			mirror->pRasterizationState = rasterizationState;
			mirror->pMultisampleState = multisampleState;
			mirror->pDepthStencilState = depthStencilState;
			mirror->pColorBlendState = colorBlendState;
			mirror->pDynamicState = dynamicState;

			if (vertexInputState)
			{
				vertexInputState->pVertexBindingDescriptions = vertexBindingDescriptions;
				vertexInputState->pVertexAttributeDescriptions = vertexAttributeDescriptions;
			}

			if (multisampleState)
			{
				multisampleState->pSampleMask = sampleMask;
			}

			if (viewportState)
			{
				viewportState->pViewports = viewports;
				viewportState->pScissors = viewportScissors;
			}

			if (colorBlendState)
			{
				colorBlendState->pAttachments = colorBlendAttachments;
			}

			if (dynamicState)
			{
				dynamicState->pDynamicStates = dynamicStates;
			}
		}

		return mirror;
	}

	VkComputePipelineCreateInfo* DeepCopy(size_t* size, uint8_t*& out, const VkComputePipelineCreateInfo& info)
	{
		return ByteCopy(size, out, info);
	}
}

template<typename T>
T* DeepCopy(size_t* size, uint8_t* out, const T& value)
{
	return Detail::DeepCopy(size, out, value);
}