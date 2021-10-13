#pragma once

#include "Shader.h"
#include "Descriptor.h"
#include <cinttypes>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <atomic>

// Represents a wrapped shader module
struct HShaderModule : public TDeferredOwnership<HShaderModule>
{
	HSourceShader					   m_SourceShader;
	HInstrumentedShader				   m_InstrumentedShader;
	VkGPUValidationShaderCreateInfoAVA m_CreateInfoAVA{};
	uint32_t						   m_SwapIndex;
};

// Diagnostic set hash
static constexpr size_t kDiagnosticSetCrossCompatabilityHash = 0ull;

// Represents the user specified push constant data per stage
struct SPushConstantStage
{
    uint32_t m_Offset;
    uint32_t m_Size;
    uint32_t m_End;
    VkShaderStageFlags m_StageFlags;
};

// Represents a wrapped pipeline layout
struct HPipelineLayout : public TDeferredOwnership<HPipelineLayout>
{
	VkPipelineLayout					 m_Layout;
	uint32_t							 m_SetLayoutCount;
	std::vector<size_t> 				 m_SetLayoutCrossCompatibilityHashes;
	uint32_t							 m_PushConstantStageRangeCount;
    SPushConstantStage                   m_PushConstantStages[16];
	uint32_t							 m_PushConstantSize;
	std::vector<SPushConstantDescriptor> m_PushConstantDescriptors;
};

enum class EPipelineType
{
	eGraphics,
	eCompute
};

// Represents a wrapped pipeline
struct HPipeline : public TDeferredOwnership<HPipeline>
{
	VkPipelineCache								 m_PipelineCache;
	VkPipeline									 m_SourcePipeline;
	std::atomic<VkPipeline>	 					 m_InstrumentedPipeline{ nullptr };
	HPipelineLayout*							 m_PipelineLayout;
	std::vector<HShaderModule*>					 m_ShaderModules;
	uint8_t		  								 m_FeatureMask;
	std::vector<uint8_t>						 m_CreationBlob;
	EPipelineType								 m_Type;
	union
	{
		VkGraphicsPipelineCreateInfo*			 m_GraphicsCreateInfo;
		VkComputePipelineCreateInfo*			 m_ComputeCreateInfo;
	};
	VkGPUValidationPipelineCreateInfoAVA		 m_CreateInfoAVA{};
	uint32_t									 m_SwapIndex;
};
