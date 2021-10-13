#include "Callbacks.h"
#include "StateTables.h"
#include <Pipeline.h>
#include <Report.h>
#include <ShaderCache.h>
#include <PipelineCompiler.h>
#include <ShaderCompiler.h>
#include <cstring>
#include <chrono>

// Specialized passes
#include <Passes/Concurrency/ResourceDataRacePass.h>
#include <Passes/DataResidency/ResourceInitializationPass.h>
#include <Private/Passes/StateVersionBreadcrumbPass.h>

VkResult VKAPI_CALL CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkCommandPool * pCommandPool)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Dedicated transfer queues are emulated
	auto info = *pCreateInfo;
	if (!(table->m_QueueFamilies[info.queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT))
	{
		info.queueFamilyIndex = device_state->m_DedicatedCopyEmulationQueueFamily;
	}

	// Pass down callchain
	VkResult result = table->m_CreateCommandPool(device, &info, pAllocator, pCommandPool);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track indices
	{
		std::lock_guard<std::mutex> guard(device_state->m_CommandFamilyIndexMutex);
		device_state->m_CommandPoolFamilyIndices[*pCommandPool] = pCreateInfo->queueFamilyIndex;
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo * pAllocateInfo, VkCommandBuffer * pCommandBuffers)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	VkResult result = table->m_AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Track indices
	uint32_t pool_family_index;
	{
		std::lock_guard<std::mutex> guard(device_state->m_CommandFamilyIndexMutex);

		pool_family_index = device_state->m_CommandPoolFamilyIndices[pAllocateInfo->commandPool];
		for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++)
		{
			device_state->m_CommandBufferFamilyIndices[pCommandBuffers[i]] = pool_family_index;
		}
	}

	return VK_SUCCESS;
}

void VKAPI_CALL FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	for (uint32_t i = 0; i < commandBufferCount; i++)
	{
		CommandStateTable* cmd_state = CommandStateTable::Get(pCommandBuffers[i]);
		if (!cmd_state)
			continue;

		// May not have been submitted
		if (cmd_state->m_Allocation)
		{
			cmd_state->m_Allocation->SkipFence();
			device_state->m_DiagnosticAllocator->PushAllocation(cmd_state->m_Allocation);
		}
	}

	// Pass down callchain
	table->m_FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

static void RestoreCommandStatePostInjection(VkCommandBuffer commandBuffer)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Restore all push constant ranges
    if (cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint])
    {
        HPipelineLayout* layout = cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint]->m_PipelineLayout;

        for (uint32_t i = 0; i < layout->m_PushConstantStageRangeCount; i++)
        {
            const SPushConstantStage &stage = layout->m_PushConstantStages[i];
            table->m_CmdPushConstants(commandBuffer, layout->m_Layout, stage.m_StageFlags, stage.m_Offset, stage.m_Size, cmd_state->m_CachedPCData);
        }
    }

	// Ignore if not compute
	// Technically allowed but not used by our GraphicsDevice
	if (cmd_state->m_ActivePipelineBindPoint != VK_PIPELINE_BIND_POINT_COMPUTE)
		return;

	// Rebind previous pipeline
	if (cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint])
	{
	    // Set internal pipeline
	    cmd_state->m_ActiveInternalPipelines[cmd_state->m_ActivePipelineBindPoint] = cmd_state->m_ActiveUnwrappedPipelines[cmd_state->m_ActivePipelineBindPoint];

	    // Rebind previous user pipeline
		table->m_CmdBindPipeline(commandBuffer, cmd_state->m_ActivePipelineBindPoint, cmd_state->m_ActiveUnwrappedPipelines[cmd_state->m_ActivePipelineBindPoint]);

		// Rebind previous sets
		for (uint32_t i = 0; i < cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint]->m_PipelineLayout->m_SetLayoutCount; i++)
		{
			const STrackedDescriptorSet& set = cmd_state->m_ActiveComputeSets[i];

			if (set.m_NativeSet)
			{
				// Decayed due to pipeline incompatibility?
				if (set.m_CrossCompatibilityHash != cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint]->m_PipelineLayout->m_SetLayoutCrossCompatibilityHashes.at(i))
					continue;

				// Note: Dynamic offset mismatch
				table->m_BindDescriptorSets(commandBuffer, cmd_state->m_ActivePipelineBindPoint, cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint]->m_PipelineLayout->m_Layout, i, 1, &set.m_NativeSet, static_cast<uint32_t>(set.m_DynamicOffsets.size()), set.m_DynamicOffsets.data());
			}
		}
	}
	else
	{
	    // Ignore the previously bound internal compute pipeline
	    // Enforces conservative rebinding
        cmd_state->m_ActiveInternalPipelines[cmd_state->m_ActivePipelineBindPoint]  = nullptr;

		// Rebind previous sets
		for (uint32_t i = 0; i < kMaxBoundDescriptorSets; i++)
		{
			const STrackedDescriptorSet& set = cmd_state->m_ActiveComputeSets[i];

			if (set.m_NativeSet)
			{
				table->m_BindDescriptorSets(commandBuffer, cmd_state->m_ActivePipelineBindPoint, set.m_OverlappedLayout, i, 1, &set.m_NativeSet, static_cast<uint32_t>(set.m_DynamicOffsets.size()), set.m_DynamicOffsets.data());
			}
		}
	}
}

VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Lazy command buffer states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);
	if (!cmd_state)
	{
		CommandStateTable::Add(commandBuffer, cmd_state = new CommandStateTable());
	}

	// Command buffer may have skipped submission
	if (cmd_state->m_Allocation)
	{
		cmd_state->m_Allocation->SkipFence();
		device_state->m_DiagnosticAllocator->PushAllocation(cmd_state->m_Allocation);
	}

	// Reset states
	cmd_state->m_ActiveRenderPass = {};

	// Reset pipelines
	for (uint32_t i = 0; i < kTrackedPipelineBindPoints; i++)
    {
        cmd_state->m_ActivePipelines[i] = nullptr;
        cmd_state->m_ActiveUnwrappedPipelines[i] = nullptr;
        cmd_state->m_ActiveInternalPipelines[i] = nullptr;
    }

	// Reset tracked sets
	for (STrackedDescriptorSet& m_ActiveComputeSet : cmd_state->m_ActiveComputeSets)
	{
		m_ActiveComputeSet.m_NativeSet = nullptr;
		m_ActiveComputeSet.m_DynamicOffsets.clear();
	}

	// Reset breadcrumb data
	cmd_state->m_DirtyBreadcrumb = false;
	for (SBreadcrumbDescriptorSet& set : cmd_state->m_BreadcrumbDescriptorSets)
    {
	    set.m_Queued = set.m_Active = nullptr;
    }

	// Pass down call chain
	VkResult result = table->m_CmdBeginCommandBuffer(commandBuffer, pBeginInfo);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Report operations must be sync
	std::lock_guard<std::mutex> report_guard(device_state->m_ReportLock);

	// Pop an allocation if active and the shader compiler has caught up
	if (device_state->m_ActiveReport
		// Has the shader compiler caught up?
		&& device_state->m_ShaderCompiler->IsCommitPushed(device_state->m_ActiveReport->m_ShaderCompilerCommit)
		
		// Has the pipeline compiler caught up?
		&& device_state->m_PipelineCompiler->IsCommitPushed(device_state->m_ActiveReport->m_PipelineCompilerCommit)
	)
	{
		cmd_state->m_ActiveFeatures = device_state->m_ActiveReport->m_BeginInfo.m_Features;

		cmd_state->m_Allocation = device_state->m_DiagnosticAllocator->PopAllocation(commandBuffer, reinterpret_cast<uint64_t>(commandBuffer));
	}
	else
	{
		// Null allocation denotes no validation
		cmd_state->m_Allocation = nullptr;
	}

	return VK_SUCCESS;
}

void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

	// Get handle
	auto handle = reinterpret_cast<HPipeline*>(pipeline);

	// Track the pipeline
	cmd_state->m_ActivePipelines[pipelineBindPoint] = handle;
	cmd_state->m_ActivePipelineBindPoint = pipelineBindPoint;

	// Instrumented pipeline may not have compiled yet
	VkPipeline instrumented_pipeline;
	if (cmd_state->m_Allocation && (instrumented_pipeline = handle->m_InstrumentedPipeline.load()) != nullptr)
	{
        cmd_state->m_ActiveUnwrappedPipelines[pipelineBindPoint] = instrumented_pipeline;
        cmd_state->m_ActiveInternalPipelines[pipelineBindPoint] = instrumented_pipeline;

		// Pass down call chain
		table->m_CmdBindPipeline(commandBuffer, pipelineBindPoint, instrumented_pipeline);

		// Bind set
		table->m_BindDescriptorSets(commandBuffer, pipelineBindPoint, handle->m_PipelineLayout->m_Layout, handle->m_PipelineLayout->m_SetLayoutCount - 1, 1, &cmd_state->m_Allocation->m_DescriptorSet, 0, nullptr);
		
		// Track set
		if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			STrackedDescriptorSet& set = cmd_state->m_ActiveComputeSets[handle->m_PipelineLayout->m_SetLayoutCount - 1];
			set.m_CrossCompatibilityHash = kDiagnosticSetCrossCompatabilityHash;
			set.m_NativeSet = cmd_state->m_Allocation->m_DescriptorSet;
			set.m_OverlappedLayout = handle->m_PipelineLayout->m_Layout;
			set.m_DynamicOffsets.clear();
		}
	}
	else
	{
        cmd_state->m_ActiveUnwrappedPipelines[pipelineBindPoint] = handle->m_SourcePipeline;
        cmd_state->m_ActiveInternalPipelines[pipelineBindPoint] = handle->m_SourcePipeline;

		// Pass down call chain
		table->m_CmdBindPipeline(commandBuffer, pipelineBindPoint, handle->m_SourcePipeline);
	}
}

void VKAPI_CALL CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
									  const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Breadcrumb tracking
    if (cmd_state->m_Allocation)
    {
        cmd_state->m_DirtyBreadcrumb = true;
        for (uint32_t i = 0; i < descriptorSetCount; i++)
        {
            // Note: Queueing multiple onto the same breadcrumb is just fine
            cmd_state->m_BreadcrumbDescriptorSets[firstSet + i].m_Queued = reinterpret_cast<HDescriptorSet *>(pDescriptorSets[i]);
        }
    }

	// Unwrap sets
	auto sets = ALLOCA_ARRAY(VkDescriptorSet, descriptorSetCount);
	for (uint32_t i = 0; i < descriptorSetCount; i++)
	{
		sets[i] = reinterpret_cast<HDescriptorSet*>(pDescriptorSets[i])->m_Set;
		
		// Track descriptor set
		if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
		{
			STrackedDescriptorSet& tracked_set = cmd_state->m_ActiveComputeSets[firstSet + i];
			tracked_set.m_CrossCompatibilityHash = reinterpret_cast<HDescriptorSet*>(pDescriptorSets[i])->m_SetLayout->m_CrossCompatibilityHash;
			tracked_set.m_NativeSet = sets[i];
			tracked_set.m_OverlappedLayout = reinterpret_cast<HPipelineLayout*>(layout)->m_Layout;

			tracked_set.m_DynamicOffsets.resize(dynamicOffsetCount);
			std::memcpy(tracked_set.m_DynamicOffsets.data(), pDynamicOffsets, sizeof(uint32_t) * dynamicOffsetCount);
		}
	}

	// Pass down callchain
	table->m_BindDescriptorSets(commandBuffer, pipelineBindPoint, reinterpret_cast<HPipelineLayout*>(layout)->m_Layout, firstSet, descriptorSetCount, sets, dynamicOffsetCount, pDynamicOffsets);
}

void VKAPI_CALL CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Cache the data
    {
        std::memcpy(&cmd_state->m_CachedPCData[offset], pValues, size);
    }

	// Pass down callchain
	table->m_CmdPushConstants(commandBuffer, reinterpret_cast<HPipelineLayout*>(layout)->m_Layout, stageFlags, offset, size, pValues);
}

void VKAPI_CALL CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, VkSubpassContents contents)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Record info
	cmd_state->m_ActiveRenderPass = *pRenderPassBegin;

	// Data race pass?
	if (auto pass = static_cast<Passes::Concurrency::ResourceDataRacePass*>(device_state->m_DiagnosticRegistry->GetPass(cmd_state->m_ActiveFeatures, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Potential Issue: We're assigning the locks before the render pass begins, however that pass could have memory barriers which safe guard it...
		pass->BeginRenderPass(commandBuffer, cmd_state->m_ActiveRenderPass);
	}

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(cmd_state->m_ActiveFeatures, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->BeginRenderPass(commandBuffer, cmd_state->m_ActiveRenderPass);

		RestoreCommandStatePostInjection(commandBuffer);
	}

	// Pass down callchain
	table->m_CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

void VKAPI_CALL CmdEndRenderPass(VkCommandBuffer commandBuffer)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdEndRenderPass(commandBuffer);

	// Data race pass?
	if (auto pass = static_cast<Passes::Concurrency::ResourceDataRacePass*>(device_state->m_DiagnosticRegistry->GetPass(cmd_state->m_ActiveFeatures, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		pass->EndRenderPass(commandBuffer, cmd_state->m_ActiveRenderPass);
	}

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(cmd_state->m_ActiveFeatures, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->EndRenderPass(commandBuffer, cmd_state->m_ActiveRenderPass);

		RestoreCommandStatePostInjection(commandBuffer);
	}

	// Considered void
	cmd_state->m_ActiveRenderPass = {};
}

static void UpdatePushConstants(DeviceDispatchTable* table, DeviceStateTable* device_state, CommandStateTable* cmd_state, VkCommandBuffer commandBuffer)
{
	// Skip if none allocated
	if (!cmd_state->m_Allocation || device_state->m_DiagnosticRegistry->GetAllocatedPushConstantUIDs() == 0)
		return;

	// Get layout handle
	HPipelineLayout* layout_handle = cmd_state->m_ActivePipelines[cmd_state->m_ActivePipelineBindPoint]->m_PipelineLayout;

	// TODO: Is this a reasonable limit?
	uint8_t pc_data[512];

	// Pass through registry
	size_t written = device_state->m_DiagnosticRegistry->UpdatePushConstants(
		commandBuffer,
		cmd_state->m_ActiveFeatures, 
		layout_handle->m_PushConstantDescriptors.data(),
		pc_data
	);

	// Skip if none needed
	if (written == 0)
		return;

	// Update all ranges
	for (uint32_t i = 0; i < layout_handle->m_PushConstantStageRangeCount; i++)
	{
		const SPushConstantStage& stage = layout_handle->m_PushConstantStages[i];
		table->m_CmdPushConstants(commandBuffer, layout_handle->m_Layout, stage.m_StageFlags, stage.m_End, layout_handle->m_PushConstantSize, pc_data);
	}
}

static void UpdateBreadcrumbs(DeviceDispatchTable* table, DeviceStateTable* device_state, CommandStateTable* cmd_state, VkPipelineBindPoint point, VkCommandBuffer cmd_buffer)
{
    if (!cmd_state->m_DirtyBreadcrumb)
        return;

    if (auto pass = static_cast<Passes::StateVersionBreadcrumbPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, Passes::kBreadcrumbPassID)))
    {
        std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

        // Merged updates
        Passes::DescriptorSetStateUpdate updates[kMaxBoundDescriptorSets];

        // Record states of bound sets
        uint32_t update_count = 0;
        for (uint32_t i = 0; i < kMaxBoundDescriptorSets; i++)
        {
            SBreadcrumbDescriptorSet& bc = cmd_state->m_BreadcrumbDescriptorSets[i];

            // Any queued?
            if (!bc.m_Queued)
                continue;

            // Needs updating?
            if (bc.m_Active && bc.m_Active->m_CommitHash == bc.m_Queued->m_CommitHash)
                continue;

            // Statistics
            device_state->m_Statistics.m_BreadcrumbDescriptorUpdates++;

            // Insert to batch
            updates[update_count++] = { i, cmd_state->m_BreadcrumbDescriptorSets[i].m_Queued };

            // Move to active
            bc.m_Active = bc.m_Queued;
            bc.m_Queued = nullptr;
        }

        // Insert the breadcrumb
        pass->BindDescriptorSets(cmd_buffer, updates, update_count);

        // Statistics
        device_state->m_Statistics.m_BreadcrumbDispatchedDescriptorUpdates++;

        // Restore the previous state
        RestoreCommandStatePostInjection(cmd_buffer);
    }

    cmd_state->m_DirtyBreadcrumb = false;
}

void VKAPI_CALL CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

	// Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VKAPI_CALL CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VKAPI_CALL CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void VKAPI_CALL CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_COMPUTE, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VKAPI_CALL CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));
    DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
    CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

    // Update PCs
    UpdateBreadcrumbs(table, device_state, cmd_state, VK_PIPELINE_BIND_POINT_COMPUTE, commandBuffer);
    UpdatePushConstants(table, device_state, cmd_state, commandBuffer);

	// Pass down callchain
	table->m_CmdDispatchIndirect(commandBuffer, buffer, offset);
}

void VKAPI_PTR CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy * pRegions)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->InitializeBuffer(commandBuffer, dstBuffer);

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy * pRegions)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < regionCount; i++) 
		{
			VkImageSubresourceRange range;
			range.aspectMask = pRegions[i].dstSubresource.aspectMask;
			range.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
			range.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
			range.layerCount = pRegions[i].dstSubresource.layerCount;
			range.levelCount = 1;

			pass->InitializeImage(commandBuffer, dstImage, range);
		}

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit * pRegions, VkFilter filter)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < regionCount; i++)
		{
			VkImageSubresourceRange range;
			range.aspectMask = pRegions[i].dstSubresource.aspectMask;
			range.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
			range.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
			range.layerCount = pRegions[i].dstSubresource.layerCount;
			range.levelCount = 1;

			pass->InitializeImage(commandBuffer, dstImage, range);
		}

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy * pRegions)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < regionCount; i++)
		{
			VkImageSubresourceRange range;
			range.aspectMask = pRegions[i].imageSubresource.aspectMask;
			range.baseArrayLayer = pRegions[i].imageSubresource.baseArrayLayer;
			range.baseMipLevel = pRegions[i].imageSubresource.mipLevel;
			range.layerCount = pRegions[i].imageSubresource.layerCount;
			range.levelCount = 1;

			pass->InitializeImage(commandBuffer, dstImage, range);
		}

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy * pRegions)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->InitializeBuffer(commandBuffer, dstBuffer);

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void * pData)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->InitializeBuffer(commandBuffer, dstBuffer);

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		pass->InitializeBuffer(commandBuffer, dstBuffer);

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue * pColor, uint32_t rangeCount, const VkImageSubresourceRange * pRanges)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < rangeCount; i++)
		{
			pass->InitializeImage(commandBuffer, image, pRanges[i]);
		}

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

void VKAPI_PTR CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue * pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange * pRanges)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < rangeCount; i++)
		{
			pass->InitializeImage(commandBuffer, image, pRanges[i]);
		}
	}
}

void VKAPI_PTR CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment * pAttachments, uint32_t rectCount, const VkClearRect * pRects)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Initialize all cleared attachments
		// Note: Disabled for now
		//		 Can't dispatch inside a render pass and it's already "initialized" by the vkCmdBeginRenderPass, needs more *thinking*
#if 0
		auto&& views = device_state->m_ResourceFramebufferSources[cmd_state->m_ActiveRenderPass.framebuffer];
		for (uint32_t i = 0; i < attachmentCount; i++)
		{
			if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
				pass->InitializeImageView(commandBuffer, views.at(pAttachments[i].colorAttachment));
			else
				pass->InitializeImageView(commandBuffer, views.at(device_state->m_ResourceRenderPassDepthSlots.at(cmd_state->m_ActiveRenderPass.renderPass)));
		}

		RestoreCommandStatePostInjection(commandBuffer);
#else
		(void)cmd_state;
#endif
	}
}

void VKAPI_PTR CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve * pRegions)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);

	// Resource initialization pass?
	if (auto pass = static_cast<Passes::DataResidency::ResourceInitializationPass*>(device_state->m_DiagnosticRegistry->GetPass(0xFFFFFFFF, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)))
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);

		// Mark all ranges as initialized
		// Merging all ranges into a single subresource range may not be possible
		for (uint32_t i = 0; i < regionCount; i++)
		{
			VkImageSubresourceRange range;
			range.aspectMask = pRegions[i].dstSubresource.aspectMask;
			range.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
			range.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
			range.layerCount = pRegions[i].dstSubresource.layerCount;
			range.levelCount = 1;

			pass->InitializeImage(commandBuffer, dstImage, range);
		}

		RestoreCommandStatePostInjection(commandBuffer);
	}
}

VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(commandBuffer));
	CommandStateTable* cmd_state = CommandStateTable::Get(commandBuffer);

	// Finalize the allocation upon completion
	VkResult result;
	if (cmd_state->m_Allocation)
	{
		// May not have dedicated transfer queue
		if (device_state->m_TransferQueue)
		{
			// Pool child access must be serial
			std::lock_guard<std::mutex> guard(device_state->m_TransferPoolMutex);

			// Prepare for the upcomming transfer
			device_state->m_DiagnosticAllocator->BeginTransferAllocation(commandBuffer, cmd_state->m_Allocation);

			// Reset buffer
			if ((result = table->m_CmdResetCommandBuffer(cmd_state->m_Allocation->m_TransferCmdBuffer, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT)) != VK_SUCCESS)
			{
				return result;
			}

			// Begin transfer recording
			VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			if ((result = table->m_CmdBeginCommandBuffer(cmd_state->m_Allocation->m_TransferCmdBuffer, &begin_info)) != VK_SUCCESS)
			{
				return result;
			}

			// Perform the transfer
			device_state->m_DiagnosticAllocator->EndTransferAllocation(cmd_state->m_Allocation->m_TransferCmdBuffer, cmd_state->m_Allocation);

			// End transfer recording
			if ((result = table->m_CmdEndCommandBuffer(cmd_state->m_Allocation->m_TransferCmdBuffer)) != VK_SUCCESS)
			{
				return result;
			}
		}
		else
		{
			// Naively perform the transfer here
			device_state->m_DiagnosticAllocator->TransferInplaceAllocation(commandBuffer, cmd_state->m_Allocation);
		}
	}

	// Pass down call chain
	if ((result = table->m_CmdEndCommandBuffer(commandBuffer)) != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

static VkSemaphore* AcquireNextSyncPoint(uint32_t submitCount, const VkSubmitInfo* pSubmits)
{
	for (uint32_t i = 0; i < submitCount; i++)
	{
		auto& submit_info = pSubmits[i];
		for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
		{
			CommandStateTable* cmd_state = CommandStateTable::Get(submit_info.pCommandBuffers[j]);
			if (cmd_state->m_Allocation)
			{
				if (cmd_state->m_Allocation->m_IsTransferSyncPoint && cmd_state->m_Allocation->m_PendingTransferSync)
				{
					cmd_state->m_Allocation->m_PendingTransferSync = false;
					return &cmd_state->m_Allocation->m_TransferSignalSemaphore;
				}

				cmd_state->m_Allocation->m_PendingTransferSync = false;
			}
		}
	}

	return nullptr;
}

VkResult VKAPI_CALL QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(queue));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(queue));

	// Emulated?
	if (queue == device_state->m_EmulatedTransferQueue)
	{
		queue = device_state->m_CopyEmulationQueue;
	}

	// Check FS emulation
    {
        // Must be serial
        std::lock_guard<std::mutex> guard(device_state->m_FSLock);

        // May not require emulation!
        auto fsqueue_it = device_state->m_FSQueues.find(queue);
        if (fsqueue_it != device_state->m_FSQueues.end())
        {
            SPendingQueueInitialization& pqi = (*fsqueue_it).second;

            // Scheduled?
            if (pqi.m_CurrentSubmission.m_CommandBuffer)
            {
                // Done recording at this point
                VkResult result;
                if ((result = table->m_CmdEndCommandBuffer(pqi.m_CurrentSubmission.m_CommandBuffer)) != VK_SUCCESS)
                {
                    return result;
                }

                // Prepare submission
                VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
                info.commandBufferCount = 1;
                info.pCommandBuffers = &pqi.m_CurrentSubmission.m_CommandBuffer;

                // Attempt to submit on the same queue
                if ((result = table->m_QueueSubmit(queue, 1, &info, pqi.m_CurrentSubmission.m_Fence)) != VK_SUCCESS)
                {
                    return result;
                }

                // Push to pending
                pqi.m_PendingSubmissions.push_back(pqi.m_CurrentSubmission);
                pqi.m_CurrentSubmission = {};

                // Reset age counter
                pqi.m_MissedFrameCounter = 0;
            }
        }
    }

	// Patch endings
	{
		SDiagnosticAllocation* last_allocation = nullptr;
		for (uint32_t i = 0; i < submitCount; i++)
		{
			auto& submit_info = pSubmits[i];
			for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
			{
				CommandStateTable* cmd_state = CommandStateTable::Get(submit_info.pCommandBuffers[j]);
				if (cmd_state->m_Allocation)
				{
					last_allocation = cmd_state->m_Allocation;
				}
			}
		}

		// Last allocation is always a sync point
		// Required for safe transfer sync points
		if (last_allocation)
			last_allocation->m_IsTransferSyncPoint = true;
	}

	// May not have async transfer capabilities
	// If we do, then each submission needs to signal that a transfer may safely begin
	VkResult result;
	if (device_state->m_TransferQueue)
	{
		// Append semaphore buffer, not very pretty
		uint32_t semaphore_offset = 0;
		auto semaphore_buffer = static_cast<VkSemaphore*>(alloca(sizeof(VkSemaphore) * 256));

		// For semaphore injection
		auto synced_submit_infos = static_cast<VkSubmitInfo*>(alloca(sizeof(VkSubmitInfo) * submitCount));
		std::memcpy(synced_submit_infos, pSubmits, sizeof(VkSubmitInfo) * submitCount);

		for (uint32_t i = 0; i < submitCount; i++)
		{
			auto& submit_info = synced_submit_infos[i];

			// Base semaphore offset
			uint32_t base_offset = semaphore_offset;
			{
				// Copy user semaphores
				std::memcpy(&semaphore_buffer[base_offset], submit_info.pSignalSemaphores, sizeof(VkSemaphore) * submit_info.signalSemaphoreCount);
				semaphore_offset += submit_info.signalSemaphoreCount;
			}

			for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
			{
				CommandStateTable* cmd_state = CommandStateTable::Get(submit_info.pCommandBuffers[j]);

				// May be selectively disabled
				if (cmd_state->m_Allocation && cmd_state->m_Allocation->m_IsTransferSyncPoint)
				{
					// Signal that a transfer can begin
					semaphore_buffer[semaphore_offset++] = cmd_state->m_Allocation->m_TransferSignalSemaphore;
				}
			}

			// Proxy semaphores
			submit_info.signalSemaphoreCount = (semaphore_offset - base_offset);
			submit_info.pSignalSemaphores = &semaphore_buffer[base_offset];
		}

		// Pass down call chain
		result = table->m_QueueSubmit(queue, submitCount, synced_submit_infos, fence);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	else
	{
		// Pass down call chain
		result = table->m_QueueSubmit(queue, submitCount, pSubmits, fence);
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	// First pass, deduce reference count
	uint32_t reference_count = 0;
	for (uint32_t i = 0; i < submitCount; i++)
	{
		auto& submit_info = pSubmits[i];
		for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
		{
			// May be selectively disabled
			reference_count += (CommandStateTable::Get(submit_info.pCommandBuffers[j])->m_Allocation != nullptr);
		}
	}

	// May either be an empty submit or validation is disabled
	if (!reference_count)
	{
		return VK_SUCCESS;
	}

	// Pop fence
	SDiagnosticFence* diagnostic_fence = device_state->m_DiagnosticAllocator->PopFence();
	diagnostic_fence->m_ReferenceCount += reference_count;

	// Sync waits for all commands
	VkPipelineStageFlags sync_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	// If async transfer is enabled, do the actual transfers
	// Otherwise create a virtual sync point for fence collection
	if (device_state->m_TransferQueue)
	{
		// Allocate submission infos
		auto transfer_submit_infos = static_cast<VkSubmitInfo*>(alloca(sizeof(VkSubmitInfo) * reference_count));

		// Transfer all allocations
		uint32_t transfer_index = 0;
		for (uint32_t i = 0; i < submitCount; i++)
		{
			auto& submit_info = pSubmits[i];
			for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
			{
				CommandStateTable* cmd_state = CommandStateTable::Get(submit_info.pCommandBuffers[j]);

				// May be selectively disabled
				if (cmd_state->m_Allocation)
				{
					auto&& transfer_info = transfer_submit_infos[transfer_index++] = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
					transfer_info.commandBufferCount = 1;
					transfer_info.pCommandBuffers = &cmd_state->m_Allocation->m_TransferCmdBuffer;
					transfer_info.pWaitDstStageMask = &sync_stage_mask;

					// Needs a sync point?
					if (cmd_state->m_Allocation->m_PendingTransferSync)
					{
						transfer_info.waitSemaphoreCount = 1;
						transfer_info.pWaitSemaphores = AcquireNextSyncPoint(submitCount - i, &submit_info);
					}
				}
			}
		}

		// Sanity check
		if (transfer_index != reference_count)
		{
			return VK_ERROR_DEVICE_LOST;
		}

		// Submit on dedicated queue
		if ((result = table->m_QueueSubmit(device_state->m_TransferQueue, reference_count, transfer_submit_infos, diagnostic_fence->m_Fence)) != VK_SUCCESS)
		{
			return result;
		}
	}
	else
	{
		// Submit on the same queue, guarenteed serial execution
		if ((result = table->m_QueueSubmit(queue, 0, nullptr, diagnostic_fence->m_Fence)) != VK_SUCCESS)
		{
			return result;
		}
	}

	// Assign handles
	for (uint32_t i = 0; i < submitCount; i++)
	{
		auto& submit_info = pSubmits[i];
		for (uint32_t j = 0; j < submit_info.commandBufferCount; j++)
		{
			CommandStateTable* cmd_state = CommandStateTable::Get(submit_info.pCommandBuffers[j]);

			// May be selectively disabled
			if (cmd_state->m_Allocation)
			{
				cmd_state->m_Allocation->SetFence(diagnostic_fence);

				// Push to allocator for deferred filtering
				device_state->m_DiagnosticAllocator->PushAllocation(cmd_state->m_Allocation);

				// Allocation no longer associated with this command buffer
				cmd_state->m_Allocation = nullptr;
			}
		}
	}

	// OK
	return VK_SUCCESS;
}

VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR * pPresentInfo)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(queue));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(queue));

	// Increment FS emulation age
    {
        // Must be serial
        std::lock_guard<std::mutex> guard(device_state->m_FSLock);

        for (auto &&kv : device_state->m_FSQueues)
        {
            SPendingQueueInitialization &pqi = kv.second;
            pqi.m_MissedFrameCounter++;
        }
    }

	// Apply diagnostic throttling
	// Note: If the GPU is producing more errors than the CPU can filter at a time then it'll keep allocating
	//		 To avoid this each present call is stochastically safe guarded
	bool message_bound = device_state->m_DiagnosticAllocator->ApplyThrottling();
	if (message_bound)
	{
		uint32_t threshold = device_state->m_DiagnosticAllocator->GetThrotteThreshold();

		if (threshold == table->m_CreateInfoAVA.m_ThrottleThresholdLimit && device_state->m_WaitForFilterMessageCounter.Next(15))
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
			{
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "The GPU is emitting validation messages faster than can be processed, the frame is throttled to compensate. Consider increasing the throttling threshold.");
			}
		}

		// Increment threshold
		if (threshold < table->m_CreateInfoAVA.m_ThrottleThresholdLimit)
		{
			threshold++;

			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
			{
				char buffer[512];
				FormatBuffer(buffer, "Increased throttling threshold to %i (limit %i)", threshold, threshold < table->m_CreateInfoAVA.m_ThrottleThresholdLimit);
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
			}

			device_state->m_DiagnosticAllocator->SetThrottleThreshold(threshold);
		}
	}

	// Let the normal auto serialization kick in if needed
	if (device_state->m_PresentAutoSerializationLastPending != device_state->m_ShaderCache->GetPendingEntries())
	{
		device_state->m_PresentAutoSerializationCounter = 0;
	}
	device_state->m_PresentAutoSerializationLastPending = device_state->m_ShaderCache->GetPendingEntries();

	// No more insertions for a while?
	if (device_state->m_PresentAutoSerializationCounter++ >= 10)
	{
		// Invoke auto serialization for pending cache entries
		device_state->m_ShaderCache->AutoSerialize();
	}

	// Pass down call chain
	VkResult result = table->m_QueuePresentKHR(queue,  pPresentInfo);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Report operations must be sync
	{
		std::lock_guard<std::mutex> report_guard(device_state->m_ReportLock);

		// Reporting?
		if (device_state->m_ActiveReport)
		{
			// Apply defragmentation, should not contend with other operations
			device_state->m_DiagnosticAllocator->ApplyDefragmentation();

			// Step time?
			double step_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - device_state->m_ActiveReport->m_LastStepRecord).count() * 1e-9;
			if (step_elapsed > device_state->m_ActiveReport->m_StepInterval)
			{
				// Insert a new step
				device_state->m_ActiveReport->m_Steps.emplace_back();

				// Insert latency
				SReportStep& step = device_state->m_ActiveReport->m_Steps.back();
				step.m_LatentUndershoots = device_state->m_ActiveReport->m_LatentUndershoots - device_state->m_ActiveReport->m_LastSteppedLatentUndershoots;
				step.m_LatentOvershoots = device_state->m_ActiveReport->m_LatentOvershoots - device_state->m_ActiveReport->m_LastSteppedLatentOvershoots;

				// Track
				device_state->m_ActiveReport->m_LastSteppedLatentUndershoots = device_state->m_ActiveReport->m_LatentUndershoots;
				device_state->m_ActiveReport->m_LastSteppedLatentOvershoots = device_state->m_ActiveReport->m_LatentOvershoots;

				// Dump diagnostic information
				device_state->m_DiagnosticRegistry->StepReport(device_state->m_ActiveReport);

				// ...
				device_state->m_ActiveReport->m_LastStepRecord = std::chrono::high_resolution_clock::now();
			}
		}
	}

	// OK
	return VK_SUCCESS;
}
