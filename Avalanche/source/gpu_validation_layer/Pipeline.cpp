#include <Callbacks.h>
#include <StateTables.h>
#include <Pipeline.h>
#include <Report.h>
#include <set>
#include <CRC.h>
#include <DeepCopy.h>
#include <PipelineCompiler.h>
#include <ShaderCompiler.h>

VkResult VKAPI_CALL CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule)
{
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Create handle
	auto handle = new HShaderModule();

	// Copy blob
	handle->m_SourceShader.m_Blob.resize(pCreateInfo->codeSize);
	std::memcpy(handle->m_SourceShader.m_Blob.data(), pCreateInfo->pCode, pCreateInfo->codeSize);
	{
		handle->m_SourceShader.m_CreateInfo = *pCreateInfo;
		handle->m_SourceShader.m_CreateInfo.pCode = reinterpret_cast<uint32_t*>(handle->m_SourceShader.m_Blob.data());
	}

	// Create original shader module
	VkResult result = table->m_CreateShaderModule(device, pCreateInfo, pAllocator, &handle->m_SourceShader.m_Module);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// May contain shader reload creation info
	auto shader_info = FindStructureType<VkGPUValidationShaderCreateInfoAVA>(pCreateInfo, VK_STRUCTURE_TYPE_GPU_VALIDATION_SHADER_CREATE_INFO_AVA);

	// Assign create info
	if (shader_info)
	{
		handle->m_CreateInfoAVA = *shader_info;
	}

	// Debug name for logging
	handle->m_SourceShader.m_Name = (shader_info && shader_info->m_Name) ? shader_info->m_Name : "<anonymous>";

	// Insert to state
	{
		std::lock_guard<std::mutex> guard(state->m_ResourceLock);
		state->m_ResourceShaderModuleSwapTable.push_back(handle);
		handle->m_SwapIndex = static_cast<uint32_t>(state->m_ResourceShaderModuleSwapTable.size() - 1);

		if (shader_info && shader_info->m_Name)
		{
			state->m_ResourceShaderModuleLUT[shader_info->m_Name] = handle;
		}
	}

	// ...
	*pShaderModule = reinterpret_cast<VkShaderModule>(handle);

	return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Pipelines are created separately for simplicity
	for (uint32_t i = 0; i < createInfoCount; i++)
	{
		// Create handle
		auto handle = new HPipeline();
		handle->m_PipelineLayout = reinterpret_cast<HPipelineLayout*>(pCreateInfos[i].layout);
		handle->m_ShaderModules.resize(pCreateInfos[i].stageCount);
		handle->m_Type = EPipelineType::eGraphics;
		handle->m_PipelineCache = pipelineCache;

		// Perform a deep copy
		{
			size_t deep_copy_size = 0;
			DeepCopy(&deep_copy_size, nullptr, pCreateInfos[i]);

			handle->m_CreationBlob.resize(deep_copy_size);
			handle->m_GraphicsCreateInfo = DeepCopy(&deep_copy_size, handle->m_CreationBlob.data(), pCreateInfos[i]);
		}

		// May contain shader reload creation info
		auto pipeline_info = FindStructureType<VkGPUValidationPipelineCreateInfoAVA>(handle->m_GraphicsCreateInfo, VK_STRUCTURE_TYPE_GPU_VALIDATION_PIPELINE_CREATE_INFO_AVA);
		if (pipeline_info)
		{
			handle->m_CreateInfoAVA = *pipeline_info;
		}
		else
		{
			handle->m_CreateInfoAVA.m_FeatureMask = 0xFFFFFFFF;
		}

		// Prepare jobs and unwrap shader handles
		for (uint32_t j = 0; j < handle->m_GraphicsCreateInfo->stageCount; j++)
		{
			handle->m_ShaderModules[j] = reinterpret_cast<HShaderModule*>(handle->m_GraphicsCreateInfo->pStages[j].module);

			// Artifact of the deep copy mechanism
			// I own the memory
			auto&& stage = const_cast<VkPipelineShaderStageCreateInfo*>(handle->m_GraphicsCreateInfo->pStages)[j];
			stage.module = handle->m_ShaderModules[j]->m_SourceShader.m_Module;
		}

		// Proxy states
		handle->m_GraphicsCreateInfo->layout = handle->m_PipelineLayout->m_Layout;

		// Create source pipeline
		VkResult result = table->m_CreateGraphicsPipelines(table->m_Device, pipelineCache, 1, handle->m_GraphicsCreateInfo, pAllocator, &handle->m_SourcePipeline);
		if (result != VK_SUCCESS)
		{
			return result;
		}

		// Ignore extensions for instrumentation
		handle->m_GraphicsCreateInfo->pNext = nullptr;

		// Insert to state
		{
			std::lock_guard<std::mutex> guard(state->m_ResourceLock);
			state->m_ResourcePipelineSwapTable.push_back(handle);
			handle->m_SwapIndex = static_cast<uint32_t>(state->m_ResourcePipelineSwapTable.size() - 1);
		}

		pPipelines[i] = reinterpret_cast<VkPipeline>(handle);
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos,
								const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Pipelines are created separately for simplicity
	for (uint32_t i = 0; i < createInfoCount; i++)
	{
		// Custom handle
		auto handle = new HPipeline();
		handle->m_PipelineLayout = reinterpret_cast<HPipelineLayout*>(pCreateInfos[i].layout);
		handle->m_ShaderModules.resize(1);
		handle->m_Type = EPipelineType::eCompute;
		handle->m_PipelineCache = pipelineCache;

		// Perform a deep copy
		{
			size_t deep_copy_size = 0;
			DeepCopy(&deep_copy_size, nullptr, pCreateInfos[i]);

			handle->m_CreationBlob.resize(deep_copy_size);
			handle->m_ComputeCreateInfo = DeepCopy(&deep_copy_size, handle->m_CreationBlob.data(), pCreateInfos[i]);
		}

		// May contain shader reload creation info
		auto pipeline_info = FindStructureType<VkGPUValidationPipelineCreateInfoAVA>(handle->m_ComputeCreateInfo, VK_STRUCTURE_TYPE_GPU_VALIDATION_PIPELINE_CREATE_INFO_AVA);
		if (pipeline_info)
		{
			handle->m_CreateInfoAVA = *pipeline_info;
		}
		else
		{
			handle->m_CreateInfoAVA.m_FeatureMask = 0xFFFFFFFF;
		}

		// Get wrapped shader
		handle->m_ShaderModules[0] = reinterpret_cast<HShaderModule*>(handle->m_ComputeCreateInfo->stage.module);

		// Proxy states
		handle->m_ComputeCreateInfo->layout = handle->m_PipelineLayout->m_Layout;
		handle->m_ComputeCreateInfo->stage.module = handle->m_ShaderModules[0]->m_SourceShader.m_Module;

		// Pass down call chain
		VkResult result = table->m_CreateComputePipelines(table->m_Device, pipelineCache, 1, handle->m_ComputeCreateInfo, pAllocator, &handle->m_SourcePipeline);
		if (result != VK_SUCCESS)
		{
			return result;
		}

		// Ignore extensions for instrumentation
		handle->m_ComputeCreateInfo->pNext = nullptr;

		// Insert to state
		{
			std::lock_guard<std::mutex> guard(state->m_ResourceLock);
			state->m_ResourcePipelineSwapTable.push_back(handle);
			handle->m_SwapIndex = static_cast<uint32_t>(state->m_ResourcePipelineSwapTable.size() - 1);
		}

		pPipelines[i] = reinterpret_cast<VkPipeline>(handle);
	}

	return VK_SUCCESS;
}

void VKAPI_CALL DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Get handle
	auto handle = reinterpret_cast<HPipeline*>(pipeline);

	// Swap last pipeline to current
	{
		std::lock_guard<std::mutex> guard(state->m_ResourceLock);

		// Sanity check
		if (handle != state->m_ResourcePipelineSwapTable[handle->m_SwapIndex])
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				char buffer[512];
				FormatBuffer(buffer, "Pipeline destruction santiy check failed");
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, buffer);
			}
		}

		// Swap with last
		if (handle->m_SwapIndex != state->m_ResourcePipelineSwapTable.size() - 1)
		{
			HPipeline* last = state->m_ResourcePipelineSwapTable.back();
			last->m_SwapIndex = handle->m_SwapIndex;
			state->m_ResourcePipelineSwapTable[last->m_SwapIndex] = last;
		}

		state->m_ResourcePipelineSwapTable.pop_back();
	}

	// Pass down call chain
	table->m_DestroyPipeline(device, handle->m_SourcePipeline, pAllocator);

	handle->Release();
}

void VKAPI_CALL DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Get handle
	auto handle = reinterpret_cast<HShaderModule*>(shaderModule);

	// Swap last pipeline to current
	{
		std::lock_guard<std::mutex> guard(state->m_ResourceLock);

		// Sanity check
		if (handle != state->m_ResourceShaderModuleSwapTable[handle->m_SwapIndex])
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				char buffer[512];
				FormatBuffer(buffer, "Shader module destruction santiy check failed");
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, buffer);
			}
		}

		// Swap with last
		if (handle->m_SwapIndex != state->m_ResourceShaderModuleSwapTable.size() - 1)
		{
			HShaderModule* last = state->m_ResourceShaderModuleSwapTable.back();
			last->m_SwapIndex = handle->m_SwapIndex;
			state->m_ResourceShaderModuleSwapTable[last->m_SwapIndex] = last;
		}

		state->m_ResourceShaderModuleSwapTable.pop_back();
	}

	// Pass down call chain
	table->m_DestroyShaderModule(device, handle->m_SourceShader.m_Module, pAllocator);

	handle->Release();
}

VkResult VKAPI_CALL SetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
    DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	auto name_info = *pNameInfo;

	// Unwrap the handle
	switch (pNameInfo->objectType)
	{
		default: break;
		case VK_OBJECT_TYPE_PIPELINE: name_info.objectHandle = reinterpret_cast<uint64_t>(reinterpret_cast<HPipeline*>(name_info.objectHandle)->m_SourcePipeline); break;
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT: name_info.objectHandle = reinterpret_cast<uint64_t>(reinterpret_cast<HPipelineLayout*>(name_info.objectHandle)->m_Layout); break;
		case VK_OBJECT_TYPE_SHADER_MODULE: name_info.objectHandle = reinterpret_cast<uint64_t>(reinterpret_cast<HShaderModule*>(name_info.objectHandle)->m_SourceShader.m_Module); break;
	}

	// Track the name
    {
        std::lock_guard<std::mutex> guard(state->m_ResourceLock);
        state->m_ResourceDebugNames[reinterpret_cast<void*>(name_info.objectHandle)] = name_info.pObjectName;
    }

	// Pass down the call chain
	return table->m_SetDebugUtilsObjectNameEXT(device, &name_info);
}

struct InstrumentationContext
{
	void Acquire()
	{
		for (HShaderModule* handle : m_Modules) handle->Acquire();
		for (HPipeline* handle : m_Pipelines) handle->Acquire();
	}

	void Release()
	{
		for (HShaderModule* handle : m_Modules) handle->Release();
		for (HPipeline* handle : m_Pipelines) handle->Release();
	}

	std::vector<HShaderModule*> m_Modules;
	std::vector<HPipeline*> m_Pipelines;
};

void RecreatePipelines(VkDevice device, VkGPUValidationReportAVA report, InstrumentationContext* context)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Instrumentation must be serial
	std::lock_guard<std::mutex> instrument_guard(state->m_InstrumentationLock);

	// Gather counts
	uint32_t graphics_pipeline_count = 0;
	uint32_t compute_pipeline_count = 0;

	// Remove previously instrumented pipeline
	for (HPipeline* pipeline : context->m_Pipelines)
	{
		if (pipeline->m_Type == EPipelineType::eGraphics) graphics_pipeline_count++;
		if (pipeline->m_Type == EPipelineType::eCompute) compute_pipeline_count++;
	}

	// Preallocate graphics pipeline infos
	GraphicsPipelineJob graphics_job;
	graphics_job.m_CreateInfos.resize(graphics_pipeline_count);

	// Preallocate compute pipeline infos
	ComputePipelineJob compute_job;
	compute_job.m_CreateInfos.resize(compute_pipeline_count);

	// Step lock to avoid early false positive commit indices
	state->m_PipelineCompiler->LockCompletionStep();

	/* GRAPHICS */
	{
		// Write all graphics pipelines
		uint32_t graphics_count = 0;
		{
			for (HPipeline* pipeline : context->m_Pipelines)
			{
				if (pipeline->m_Type != EPipelineType::eGraphics)
					continue;

				// Copy create info
				VkGraphicsPipelineCreateInfo& info = graphics_job.m_CreateInfos[graphics_count++];
				info = *pipeline->m_GraphicsCreateInfo;

#if PIPELINE_COMPILER_DEBUG
                auto source = new PipelineJobDebugSource{};
                source->m_SourcePipeline = pipeline;

                info.pNext = source;
#endif

				// Just override for now
				graphics_job.m_Cache = pipeline->m_PipelineCache;

				// Assign new modules
				for (uint32_t j = 0; j < info.stageCount; j++)
				{
					const_cast<VkPipelineShaderStageCreateInfo*>(info.pStages)[j].module = pipeline->m_ShaderModules[j]->m_InstrumentedShader.m_Module;
				}
			}
		}

		// Create instrumented graphics pipelines
		if (graphics_count)
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
			{
				char buffer[512];
				FormatBuffer(buffer, "Recreating %i [GRAPHICS] pipelines", graphics_count);
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
			}

			// Push for async compilation
			state->m_PipelineCompiler->Push(std::move(graphics_job), [=](uint64_t head, VkResult result, VkPipeline* pipelines)
			{
				if (result != VK_SUCCESS)
				{
					return;
				}

				// Write instrumented graphics pipelines
				uint32_t counter = 0;
				for (HPipeline* pipeline : context->m_Pipelines)
				{
					if (pipeline->m_Type != EPipelineType::eGraphics)
						continue;

					pipeline->m_InstrumentedPipeline = pipelines[counter++];
				}

				if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
				{
					char buffer[512];
					FormatBuffer(buffer, "Finished recreating %i [GRAPHICS] pipelines", graphics_count);
					table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
				}

				if (state->m_PipelineCompiler->IsCommitPushed(head, report->m_PipelineCompilerCommit))
				{
					context->Release();
					delete context;
				}
			});
		}
	}

	/* COMPUTE */
	{
		// Write all compute pipelines
		uint32_t compute_count = 0;
		{
			for (HPipeline* pipeline : context->m_Pipelines)
			{
				if (pipeline->m_Type != EPipelineType::eCompute)
					continue;

				// Copy create info
				VkComputePipelineCreateInfo& info = compute_job.m_CreateInfos[compute_count++];
				info = *pipeline->m_ComputeCreateInfo;

				// Just override for now
				compute_job.m_Cache = pipeline->m_PipelineCache;

				// Assign new module
				info.stage.module = pipeline->m_ShaderModules[0]->m_InstrumentedShader.m_Module;
			}
		}

		// Create instrumented graphics pipelines
		if (compute_count)
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
			{
				char buffer[512];
				FormatBuffer(buffer, "Recreating %i [COMPUTE] pipelines...", compute_count);
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
			}

			// Push for async compilation
			state->m_PipelineCompiler->Push(std::move(compute_job), [=](uint64_t head, VkResult result, VkPipeline* pipelines)
			{
				if (result != VK_SUCCESS)
				{
					return;
				}

				// Write instrumented compute pipelines
				uint32_t counter = 0;
				for (HPipeline* pipeline : context->m_Pipelines)
				{
					if (pipeline->m_Type != EPipelineType::eCompute)
						continue;

					pipeline->m_InstrumentedPipeline = pipelines[counter++];
				}

				if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
				{
					char buffer[512];
					FormatBuffer(buffer, "Finished recreating %i [COMPUTE] pipelines", compute_count);
					table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
				}

				if (state->m_PipelineCompiler->IsCommitPushed(head, report->m_PipelineCompilerCommit))
				{
					context->Release();
					delete context;
				}
			});
		}
	}

	// Set pipeline commit
	report->m_PipelineCompilerCommit = state->m_PipelineCompiler->GetCommit();

	// Good to go!
	state->m_PipelineCompiler->UnlockCompletionStep();
}

VkResult InstrumentPipelines(VkDevice device, VkGPUValidationReportAVA report)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Create a snapshot used for instrumentation
	auto context = new InstrumentationContext();
	{
		// Lock everything!
		std::lock_guard<std::mutex> guard(state->m_ResourceLock);

		// Copy resources
		context->m_Modules = state->m_ResourceShaderModuleSwapTable;
		context->m_Pipelines = state->m_ResourcePipelineSwapTable;

		// Acquire usages
		// Required for safe deferred compilation
		for (HShaderModule* handle : context->m_Modules) handle->Acquire();
		for (HPipeline* handle : context->m_Pipelines) handle->Acquire();
	}

	// Remove previously instrumented pipeline
	for (HPipeline* pipeline : context->m_Pipelines)
	{
		VkPipeline previous = pipeline->m_InstrumentedPipeline.exchange(nullptr);

		// Destroy previously instrumented pipeline
		if (previous)
		{
			table->m_DestroyPipeline(device, previous, nullptr);
		}
	}

	// Prepare all jobs
	std::vector<ShaderJob> jobs(context->m_Modules.size());
	for (size_t i = 0; i < context->m_Modules.size(); i++)
	{
		HShaderModule* shader_module = context->m_Modules[i];

		// Deduce active features
		uint32_t feature_set = report->m_BeginInfo.m_Features; // &pipeline->m_CreateInfoAVA.m_FeatureMask;

		// Prepare job
		ShaderJob& job = jobs[i];
		job.m_SourceShader = &shader_module->m_SourceShader;
		job.m_InstrumentedShader = &shader_module->m_InstrumentedShader;
		job.m_Features = feature_set;
	}

	// Push for async compilation
	state->m_ShaderCompiler->Push(jobs.data(), static_cast<uint32_t>(jobs.size()), [=](uint64_t /*head*/, VkResult result)
	{
		// May have failed
		if (result != VK_SUCCESS)
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				char buffer[512];
				FormatBuffer(buffer, "Shader instrumentation failed, one or more jobs failed");
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, buffer);
			}

			return;
		}

		// Create using instrumented shader modules
		RecreatePipelines(device, report, context);
	});

	// Set shader commit
	report->m_ShaderCompilerCommit = state->m_ShaderCompiler->GetCommit();

	// OK
	return VK_SUCCESS;
}
