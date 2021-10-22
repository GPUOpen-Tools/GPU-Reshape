#include <Callbacks.h>
#include <StateTables.h>
#include <Pipeline.h>
#include <Report.h>
#include <DiagnosticAllocator.h>
#include <DiagnosticRegistry.h>
#include <set>
#include <CRC.h>
#include <cstring>

VkResult VKAPI_CALL CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout)
{
	DeviceDispatchTable* table	   = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Custom handle
	auto handle = new HPipelineLayout();

	// Last layout must be allocator
	auto set_layouts = ALLOCA_ARRAY(VkDescriptorSetLayout, pCreateInfo->setLayoutCount + 1);

	// Unwrap sets
	for (uint32_t i = 0; i < pCreateInfo->setLayoutCount; i++)
	{
		handle->m_SetLayoutCrossCompatibilityHashes.push_back(reinterpret_cast<HDescriptorSetLayout*>(pCreateInfo->pSetLayouts[i])->m_CrossCompatibilityHash);
		set_layouts[i] = reinterpret_cast<HDescriptorSetLayout*>(pCreateInfo->pSetLayouts[i])->m_SetLayout;
	}
	
	// Copy diagnostic set layout
	handle->m_SetLayoutCrossCompatibilityHashes.push_back(kDiagnosticSetCrossCompatabilityHash);
	set_layouts[pCreateInfo->setLayoutCount] = device_state->m_DiagnosticAllocator->GetSharedSetLayout();

	// Get number of pass push constants
	uint32_t pass_pc_count = 0;
	device_state->m_DiagnosticRegistry->EnumeratePushConstants(nullptr, &pass_pc_count);

	// Get push constants
	auto pass_pcs = ALLOCA_ARRAY(SDiagnosticPushConstantInfo, pass_pc_count);
	device_state->m_DiagnosticRegistry->EnumeratePushConstants(pass_pcs, &pass_pc_count);

	// Current offset
	uint32_t offset = 0;

	// Write pass push constants
	handle->m_PushConstantDescriptors.resize(pass_pc_count);
	for (uint32_t i = 0; i < pass_pc_count; i++)
	{
		auto&& desc = pass_pcs[i];

		// Write descriptor
		SPushConstantDescriptor& descriptor = handle->m_PushConstantDescriptors[desc.m_UID];
		descriptor.m_DataOffset = offset;

		offset += FormatToSize(desc.m_Format);
	}

	// Track user PC endings
	handle->m_PushConstantSize = offset;
	handle->m_PushConstantStageRangeCount = std::max<uint32_t>((pass_pc_count > 0), pCreateInfo->pushConstantRangeCount);

	// Copy top push constants
	auto ranges = ALLOCA_ARRAY(VkPushConstantRange, handle->m_PushConstantStageRangeCount + 1); // +1 for missing stages
	std::memcpy(ranges, pCreateInfo->pPushConstantRanges, sizeof(VkPushConstantRange) * pCreateInfo->pushConstantRangeCount);

	// No push constants?
	if (pCreateInfo->pushConstantRangeCount)
	{
		for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++)
		{
			auto&& range = ranges[i];

			SPushConstantStage stage{};
			stage.m_Offset = range.offset;
			stage.m_Size = range.size;
			stage.m_End = stage.m_Offset + stage.m_Size;
			stage.m_StageFlags = range.stageFlags;
			handle->m_PushConstantStages[i] = stage;

			range.size += handle->m_PushConstantSize;
		}

		// Ignore compute
		if (pass_pc_count && !(ranges[0].stageFlags & VK_SHADER_STAGE_COMPUTE_BIT))
		{
			// Get the stages which are accounted for
			uint32_t accounted_stages = 0;
			for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++)
			{
				accounted_stages |= pCreateInfo->pPushConstantRanges[i].stageFlags;
			}

			// Determine which are missing
			uint32_t missing_stages = VK_SHADER_STAGE_GEOMETRY_BIT & ~accounted_stages;

			// Any missing?
			if (missing_stages)
			{
				// Get the offset at which the missing ranges should start
				uint32_t missing_offset = 0;
				for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++)
				{
					missing_offset = std::max(missing_offset, ranges[i].offset + ranges[i].size);
				}

				// Create range
				auto&& range = ranges[pCreateInfo->pushConstantRangeCount];
				range.offset = missing_offset;
				range.size = handle->m_PushConstantSize;
				range.stageFlags = missing_stages;

				// Describe stage range
                SPushConstantStage stage{};
                stage.m_Offset = range.offset;
                stage.m_Size = range.size;
                stage.m_End = stage.m_Offset;
                stage.m_StageFlags = range.stageFlags;

				handle->m_PushConstantStages[pCreateInfo->pushConstantRangeCount] = stage;
				handle->m_PushConstantStageRangeCount++;
			}

		}
	}
	else if (pass_pc_count)
	{
		auto&& range = ranges[pCreateInfo->pushConstantRangeCount];
		range.offset = 0;
		range.size = handle->m_PushConstantSize;
		range.stageFlags = VK_SHADER_STAGE_ALL;

        // Describe stage range
        SPushConstantStage stage{};
        stage.m_Offset = range.offset;
        stage.m_Size = range.size;
        stage.m_End = stage.m_Offset;
        stage.m_StageFlags = range.stageFlags;
		handle->m_PushConstantStages[0] = stage;
	}

	// Proxy sets
	VkPipelineLayoutCreateInfo create_info = *pCreateInfo;
	create_info.pSetLayouts = set_layouts;
	create_info.setLayoutCount++;
	create_info.pPushConstantRanges = ranges;
	create_info.pushConstantRangeCount = handle->m_PushConstantStageRangeCount;
	handle->m_SetLayoutCount = create_info.setLayoutCount;

	// Pass down call chain
	VkResult result = table->m_CreatePipelineLayout(device, &create_info, pAllocator, &handle->m_Layout);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	*pPipelineLayout = reinterpret_cast<VkPipelineLayout>(handle);
	return VK_SUCCESS;
}

void VKAPI_CALL DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	auto handle = reinterpret_cast<HPipelineLayout*>(pipelineLayout);

	// Pass down callchain
	table->m_DestroyPipelineLayout(device, handle->m_Layout, pAllocator);

	handle->Release();
}

VkResult VKAPI_CALL CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Create handle
	auto handle = new HDescriptorPool();

	// Get number of pass descriptors
	uint32_t pass_descriptor_count = 0;
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

	// Get pass descriptors
	auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

	// Copy top pool sizes
	auto pool_sizes = ALLOCA_ARRAY(VkDescriptorPoolSize, pCreateInfo->poolSizeCount + pass_descriptor_count);
	std::memcpy(pool_sizes, pCreateInfo->pPoolSizes, sizeof(VkDescriptorPoolSize) * pCreateInfo->poolSizeCount);

	// Copy create info
	auto create_info = *pCreateInfo;
	create_info.pPoolSizes = pool_sizes;

	for (uint32_t i = 0; i < pass_descriptor_count; i++)
	{
		uint32_t pool_index = UINT32_MAX;
		for (uint32_t j = 0; j < create_info.poolSizeCount; j++)
		{
			if (create_info.pPoolSizes[j].type == pass_descriptors[i].m_DescriptorType)
			{
				pool_index = j;
				break;
			}
		}

		// No relevant pool size?
		if (pool_index == UINT32_MAX)
		{
			auto&& size = pool_sizes[create_info.poolSizeCount++];
			size.descriptorCount = create_info.maxSets; // One per set
			size.type = pass_descriptors[i].m_DescriptorType;
		}
		else
		{
			pool_sizes[pool_index].descriptorCount += create_info.maxSets;
		}
	}

	// Pass down call chain
	VkResult result = table->m_CreateDescriptorPool(device, &create_info, pAllocator, &handle->m_Pool);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Insert to state
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		device_state->m_ResourceDescriptorPoolSwaptable.push_back(handle);
		handle->m_SwapIndex = static_cast<uint32_t>(device_state->m_ResourceDescriptorPoolSwaptable.size() - 1);
	}

	// OK
	*pDescriptorPool = reinterpret_cast<VkDescriptorPool>(handle);
	return VK_SUCCESS;
}

VkResult VKAPI_CALL ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap
	auto handle = reinterpret_cast<HDescriptorPool*>(descriptorPool);

	// Pass down call chain
	VkResult result = table->m_ResetDescriptorPool(device, handle->m_Pool, flags);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// While descriptor pools usages are serial, this extension may concurrently iterate them
	std::lock_guard<std::mutex> guard(handle->m_InternalLock);

	// Free all sets
	for (HDescriptorSet* set : handle->m_Sets)
	{
		// Free storage data
		device_state->m_DiagnosticRegistry->DestroyDescriptors(set);
		set->Release();
	}

	// OK
	handle->m_Sets.clear();
	return VK_SUCCESS;
}

void VKAPI_CALL DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks * pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap
	auto handle = reinterpret_cast<HDescriptorPool*>(descriptorPool);

	// Swap last pipeline to current
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		if (handle->m_SwapIndex != device_state->m_ResourceDescriptorPoolSwaptable.size() - 1)
		{
			HDescriptorPool* last = device_state->m_ResourceDescriptorPoolSwaptable.back();
			device_state->m_ResourceDescriptorPoolSwaptable.pop_back();

			last->m_SwapIndex = handle->m_SwapIndex;
			device_state->m_ResourceDescriptorPoolSwaptable[last->m_SwapIndex] = last;
		}
	}

	// Free all sets
	{
		// While descriptor pools usages are serial, this extension may concurrently iterate them
		std::lock_guard<std::mutex> guard(handle->m_InternalLock);
		for (HDescriptorSet* set : handle->m_Sets)
		{
			// Free storage data
			device_state->m_DiagnosticRegistry->DestroyDescriptors(set);
			set->Release();
		}
	}

	// Pass down call chain
	table->m_DestroyDescriptorPool(device, handle->m_Pool, pAllocator);

	// Clean up
	handle->Release();
}

VkResult VKAPI_CALL CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));
	
	// Create handle
	auto handle = new HDescriptorSetLayout();
	handle->m_TopCount = pCreateInfo->bindingCount;
	handle->m_CrossCompatibilityHash = 0ull;

	// Get number of pass descriptors
	uint32_t pass_descriptor_count = 0;
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

	// Get pass descriptors
	auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

	// Copy top bindings
	auto bindings = ALLOCA_ARRAY(VkDescriptorSetLayoutBinding, pCreateInfo->bindingCount + pass_descriptor_count);
	std::memcpy(bindings, pCreateInfo->pBindings, sizeof(VkDescriptorSetLayoutBinding) * pCreateInfo->bindingCount);

	// Find the top binding and accomodate hash
	handle->m_TopBinding = 0;
	for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++)
	{
		handle->m_TopBinding = std::max(handle->m_TopBinding, bindings[i].binding + 1);
	}

	// Write pass descriptors into layout
	for (uint32_t i = 0; i < pass_descriptor_count; i++)
	{
		auto&& binding = bindings[pCreateInfo->bindingCount + i] = {};
		binding.descriptorCount = 1;
		binding.descriptorType = pass_descriptors[i].m_DescriptorType;
		binding.binding = handle->m_TopBinding + pass_descriptors[i].m_UID;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
	}

	// Copy entry templates for later reflection
	handle->m_Descriptors.resize(pCreateInfo->bindingCount + pass_descriptor_count);
	for (size_t i = 0; i < pCreateInfo->bindingCount + pass_descriptor_count; i++)
	{
		SDescriptor& descriptor = handle->m_Descriptors[i];
		descriptor.m_DescriptorType = bindings[i].descriptorType;
		descriptor.m_DescriptorCount = bindings[i].descriptorCount;
		descriptor.m_DstBinding = bindings[i].binding;
		descriptor.m_BlobOffset = UINT64_MAX;

        // Note: Immutable samplers are ignored
        CombineHash(handle->m_CrossCompatibilityHash, descriptor.m_DstBinding);
        CombineHash(handle->m_CrossCompatibilityHash, descriptor.m_DescriptorCount);
        CombineHash(handle->m_CrossCompatibilityHash, descriptor.m_DescriptorType);
	}

	// Proxy bindings
	auto create_info = *pCreateInfo;
	create_info.pBindings = bindings;
	create_info.bindingCount = pCreateInfo->bindingCount + pass_descriptor_count;

	// Pass down callchain
	VkResult result = table->m_CreateDescriptorSetLayout(device, &create_info, pAllocator, &handle->m_SetLayout);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// OK
	*pSetLayout = reinterpret_cast<VkDescriptorSetLayout>(handle);
	return VK_SUCCESS;
}

void VKAPI_CALL DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout setLayout, const VkAllocationCallbacks * pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Unwrap
	auto handle = reinterpret_cast<HDescriptorSetLayout*>(setLayout);

	// Pass down call chain
	table->m_DestroyDescriptorSetLayout(device, handle->m_SetLayout, pAllocator);
	
	// Cleanup
	handle->Release();
}

void VKAPI_CALL DestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate _template, const VkAllocationCallbacks * pAllocator)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get handle
	auto handle = reinterpret_cast<HDescriptorUpdateTemplate*>(_template);

	// Destroy underlying template
	table->m_DestroyDescriptorUpdateTemplate(device, handle->m_Template, pAllocator);

	// Cleanup
	handle->Release();
}

VkResult VKAPI_CALL AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap pool
	auto pool_handle = reinterpret_cast<HDescriptorPool*>(pAllocateInfo->descriptorPool);

	// While descriptor pools usages are serial, this extension may concurrently iterate them
	std::lock_guard<std::mutex> guard(pool_handle->m_InternalLock);

	for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++)
	{
		// Create handle
		auto handle = new HDescriptorSet();
		handle->m_SetLayout = reinterpret_cast<HDescriptorSetLayout*>(pAllocateInfo->pSetLayouts[i]);
		handle->m_Storage.resize(device_state->m_DiagnosticRegistry->GetAllocatedDescriptorStorageUIDs());
		handle->m_TrackedWrites.resize(handle->m_SetLayout->m_TopBinding);
		handle->m_Valid = false;
        handle->m_CommitIndex = 0;
        handle->m_CommitHash = 0;

		// Proxy set layouts
		auto allocate_info = *pAllocateInfo;
		allocate_info.pSetLayouts = &handle->m_SetLayout->m_SetLayout;
		allocate_info.descriptorSetCount = 1;
		allocate_info.descriptorPool = pool_handle->m_Pool;

		// Pass down call chain
		VkResult result = table->m_AllocateDescriptorSets(device, &allocate_info, &handle->m_Set);
		if (result != VK_SUCCESS)
		{
			return result;
		}

		// Allocate storage data
		device_state->m_DiagnosticRegistry->CreateDescriptors(handle);

		// Track
		pool_handle->m_Sets.push_back(handle);

		// Wrap
		pDescriptorSets[i] = reinterpret_cast<VkDescriptorSet>(handle);
	}

	return VK_SUCCESS;
}

VkResult VKAPI_CALL FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet * pDescriptorSets)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap pool
	auto pool_handle = reinterpret_cast<HDescriptorPool*>(descriptorPool);

	// While descriptor pools usages are serial, this extension may concurrently iterate them
	std::lock_guard<std::mutex> guard(pool_handle->m_InternalLock);

	// Unwrap sets
	auto sets = ALLOCA_ARRAY(VkDescriptorSet, descriptorSetCount);
	for (uint32_t i = 0; i < descriptorSetCount; i++)
	{
		auto set_handle = reinterpret_cast<HDescriptorSet*>(pDescriptorSets[i]);

		// Free storage data
		device_state->m_DiagnosticRegistry->DestroyDescriptors(set_handle);

		// Unwrap
		sets[i] = set_handle->m_Set;

		// Erase instance
		pool_handle->m_Sets.erase(std::find(pool_handle->m_Sets.begin(), pool_handle->m_Sets.end(), set_handle));
	}
	
	// Pass down callchain
	return table->m_FreeDescriptorSet(device, pool_handle->m_Pool, descriptorSetCount, sets);
}

static size_t DescriptorTypeToOffset(VkDescriptorType type)
{
	switch (type)
	{
	default:
		// Unsupported feature used by pass
		return 0;

	case VK_DESCRIPTOR_TYPE_SAMPLER:
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		return sizeof(VkDescriptorImageInfo);

	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
		return sizeof(VkBufferView);

	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
		return sizeof(VkDescriptorBufferInfo);
	}
}

void VKAPI_CALL UpdateUnpackedDescriptorWrites(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet * pDescriptorCopies)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	/*
	    Note: Mixed set updates are not supported!
		      Would be a royal pain in the ass...
	*/

	// Get number of pass descriptors
	uint32_t pass_descriptor_count = 0;
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

	// Get pass descriptors
	auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

	// Copy write top bindings
	auto writes = ALLOCA_ARRAY(VkWriteDescriptorSet, descriptorWriteCount + pass_descriptor_count);
	std::memcpy(writes, pDescriptorWrites, sizeof(VkWriteDescriptorSet) * descriptorWriteCount);

	// Descriptor templates
	auto templates = ALLOCA_ARRAY(SDescriptor, descriptorWriteCount + pass_descriptor_count);

	// Descriptor blob
	auto blob = ALLOCA_ARRAY(uint8_t, (descriptorWriteCount + pass_descriptor_count) * sizeof(VkDescriptorBufferInfo));

	// Current offset
	size_t offset = 0;

	// Unwrap write sets and update handles
	HDescriptorSet* set_handle = nullptr;
	for (uint32_t i = 0; i < descriptorWriteCount; i++)
	{
		auto write_set_handle = reinterpret_cast<HDescriptorSet*>(writes[i].dstSet);

		// Mixed not supported
		if (!set_handle)
        {
            set_handle = write_set_handle;
            set_handle->m_CommitHash = 0;
        }
		else if (set_handle != write_set_handle)
		{
			// TODO: Error handling? There's no return code...
			return;
		}

		// Unwrap
		writes[i].dstSet = set_handle->m_Set;

		// Copy template
		templates[i].m_BlobOffset = offset;
		templates[i].m_DstBinding = writes[i].dstBinding;
		templates[i].m_DescriptorCount = writes[i].descriptorCount;
		templates[i].m_DescriptorType = writes[i].descriptorType;

		// Prepare tracked write
		STrackedWrite tracked_write;
		tracked_write.m_DstBinding = writes[i].dstBinding;
		tracked_write.m_DstArrayElement = writes[i].dstArrayElement;
		tracked_write.m_DescriptorCount = writes[i].descriptorCount;
		tracked_write.m_DescriptorType = writes[i].descriptorType;

		// Copy descriptors to blob
		switch (writes[i].descriptorType)
		{
		default:
			// Unsupported feature used by pass
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			tracked_write.m_ImageInfo = *writes[i].pImageInfo;
			*reinterpret_cast<VkDescriptorImageInfo*>(&blob[offset]) = *writes[i].pImageInfo;
			offset += sizeof(VkDescriptorImageInfo);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			tracked_write.m_TexelBufferView = *writes[i].pTexelBufferView;
			*reinterpret_cast<VkBufferView*>(&blob[offset]) = *writes[i].pTexelBufferView;
			offset += sizeof(VkBufferView);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			tracked_write.m_BufferInfo = *writes[i].pBufferInfo;
			*reinterpret_cast<VkDescriptorBufferInfo*>(&blob[offset]) = *writes[i].pBufferInfo;
			offset += sizeof(VkDescriptorBufferInfo);
			break;
		}

		// Hash the write
		CombineHash(set_handle->m_CommitHash, ComputeCRC64Buffer(tracked_write));

		// Track
		set_handle->m_TrackedWrites[writes[i].dstBinding] = tracked_write;
	}

	// Prepare pass templates
	for (uint32_t i = 0; i < pass_descriptor_count; i++)
	{
		auto&& _template = templates[descriptorWriteCount + i];
		_template.m_DescriptorCount = 1;
		_template.m_DstBinding = set_handle->m_SetLayout->m_TopBinding + pass_descriptors[i].m_UID;
		_template.m_DescriptorType = pass_descriptors[i].m_DescriptorType;
		_template.m_BlobOffset = offset;
		_template.m_ArrayStride = std::max(16u, FormatToSize(pass_descriptors[i].m_ElementFormat));

		// Apply offset
		offset += DescriptorTypeToOffset(_template.m_DescriptorType);
	}

	// Copy copy top bindings
	auto copies = ALLOCA_ARRAY(VkCopyDescriptorSet, descriptorCopyCount);
	std::memcpy(copies, pDescriptorCopies, sizeof(VkCopyDescriptorSet) * descriptorCopyCount);

	// Unwrap copy sets
	for (uint32_t i = 0; i < descriptorCopyCount; i++)
	{
		copies[i].dstSet = reinterpret_cast<HDescriptorSet*>(copies[i].dstSet)->m_Set;
	}

	// Active report?
	uint32_t feature_set = 0x0;
	if (device_state->m_ActiveReport)
	{
		feature_set = device_state->m_ActiveReport->m_BeginInfo.m_Features;
	}

	// Update pass descriptor data
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		device_state->m_DiagnosticRegistry->UpdateDescriptors(set_handle, true, feature_set, templates, &templates[descriptorWriteCount], descriptorWriteCount, blob);
	}

	// Write pass descriptor templates
	for (uint32_t i = descriptorWriteCount; i < descriptorWriteCount + pass_descriptor_count; i++)
	{
		auto&& write = writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.dstSet = set_handle->m_Set;
		write.dstBinding = templates[i].m_DstBinding;
		write.descriptorType = templates[i].m_DescriptorType;
		write.descriptorCount = 1;

		// Copy from blob
		switch (writes[i].descriptorType)
		{
		default:
			// Unsupported feature used by pass
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			write.pImageInfo = reinterpret_cast<VkDescriptorImageInfo*>(&blob[templates[i].m_BlobOffset]);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			write.pTexelBufferView = reinterpret_cast<VkBufferView*>(&blob[templates[i].m_BlobOffset]);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			write.pBufferInfo = reinterpret_cast<VkDescriptorBufferInfo*>(&blob[templates[i].m_BlobOffset]);
			break;
		}
	}

	// Pass down callchain
	table->m_UpdateDescriptorSets(device, descriptorWriteCount + pass_descriptor_count, writes, descriptorCopyCount, copies);

	// Considered valid at this point
	set_handle->m_Valid = true;

	// Increment commit
	set_handle->m_CommitIndex++;
}

void VKAPI_CALL InstrumentedLiveSet(VkDevice device, HDescriptorSet* set_handle, uint32_t feature_set, uint32_t descriptorWriteCount, const STrackedWrite * pDescriptorWrites)
{
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	/*
		Note: Mixed set updates are not supported!
			  Would be a royal pain in the ass...
	*/

	// Get number of pass descriptors
	uint32_t pass_descriptor_count = 0;
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

	// Get pass descriptors
	auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

	// Descriptor templates
	auto templates = ALLOCA_ARRAY(SDescriptor, descriptorWriteCount + pass_descriptor_count);

	// Descriptor blob
	auto blob = ALLOCA_ARRAY(uint8_t, (descriptorWriteCount + pass_descriptor_count) * sizeof(VkDescriptorBufferInfo));

	// Current offset
	size_t offset = 0;

	// Unwrap write sets and update handles
	for (uint32_t i = 0; i < descriptorWriteCount; i++)
	{
		// Copy template
		templates[i].m_BlobOffset = offset;
		templates[i].m_DstBinding = pDescriptorWrites[i].m_DstBinding;
		templates[i].m_DescriptorCount = pDescriptorWrites[i].m_DescriptorCount;
		templates[i].m_DescriptorType = pDescriptorWrites[i].m_DescriptorType;

		// Copy descriptors to blob
		switch (pDescriptorWrites[i].m_DescriptorType)
		{
		default:
			// Unsupported feature used by pass
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			*reinterpret_cast<VkDescriptorImageInfo*>(&blob[offset]) = pDescriptorWrites[i].m_ImageInfo;
			offset += sizeof(VkDescriptorImageInfo);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			*reinterpret_cast<VkBufferView*>(&blob[offset]) = pDescriptorWrites[i].m_TexelBufferView;
			offset += sizeof(VkBufferView);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			*reinterpret_cast<VkDescriptorBufferInfo*>(&blob[offset]) = pDescriptorWrites[i].m_BufferInfo;
			offset += sizeof(VkDescriptorBufferInfo);
			break;
		}
	}

	// Prepare pass templates
	for (uint32_t i = 0; i < pass_descriptor_count; i++)
	{
		auto&& _template = templates[descriptorWriteCount + i];
		_template.m_DescriptorCount = 1;
		_template.m_DstBinding = set_handle->m_SetLayout->m_TopBinding + pass_descriptors[i].m_UID;
		_template.m_DescriptorType = pass_descriptors[i].m_DescriptorType;
		_template.m_BlobOffset = offset;
		_template.m_ArrayStride = std::max(16u, FormatToSize(pass_descriptors[i].m_ElementFormat));

		// Apply offset
		offset += DescriptorTypeToOffset(_template.m_DescriptorType);
	}

	// Update pass descriptor data
	device_state->m_DiagnosticRegistry->UpdateDescriptors(set_handle, false, feature_set, templates, &templates[descriptorWriteCount], descriptorWriteCount, blob);
}

void VKAPI_CALL UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet * pDescriptorCopies)
{
	UpdateUnpackedDescriptorWrites(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult VKAPI_CALL CreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorUpdateTemplate * pDescriptorUpdateTemplate)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap set layout
	auto set_layout = reinterpret_cast<HDescriptorSetLayout*>(pCreateInfo->descriptorSetLayout);

	// Create handle
	auto handle = new HDescriptorUpdateTemplate();
	handle->m_TopBlobSize = 0;
	handle->m_TopCount = pCreateInfo->descriptorUpdateEntryCount;

	// Get number of pass descriptors
	uint32_t pass_descriptor_count = 0;
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

	// Get pass descriptors
	auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
	device_state->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

	// Copy top update entries
	auto entries = ALLOCA_ARRAY(VkDescriptorUpdateTemplateEntry, pCreateInfo->descriptorUpdateEntryCount + pass_descriptor_count);
	std::memcpy(entries, pCreateInfo->pDescriptorUpdateEntries, sizeof(VkDescriptorUpdateTemplateEntry) * pCreateInfo->descriptorUpdateEntryCount);

	// Base offset
	size_t offset = 0;
	if (pCreateInfo->descriptorUpdateEntryCount)
	{
		auto& last = entries[pCreateInfo->descriptorUpdateEntryCount - 1];
		offset = last.offset + DescriptorTypeToOffset(last.descriptorType);
	}

	// Track the user blob size
	handle->m_TopBlobSize = offset;

	// Write pass descriptors into entries
	for (uint32_t i = 0; i < pass_descriptor_count; i++)
	{
		auto&& entry = entries[pCreateInfo->descriptorUpdateEntryCount + i] = {};
		entry.descriptorCount = 1;
		entry.descriptorType = pass_descriptors[i].m_DescriptorType;
		entry.dstBinding = set_layout->m_TopBinding + pass_descriptors[i].m_UID;
		entry.stride = 0;
		entry.offset = offset;

		// Append offset
		offset += DescriptorTypeToOffset(entry.descriptorType);
	}

	// Copy entry templates for later reflection
	handle->m_Descriptors.resize(pCreateInfo->descriptorUpdateEntryCount + pass_descriptor_count);
	for (size_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount + pass_descriptor_count; i++)
	{
		SDescriptor& descriptor = handle->m_Descriptors[i];
		descriptor.m_DescriptorType = entries[i].descriptorType;
		descriptor.m_DescriptorCount = entries[i].descriptorCount;
		descriptor.m_DstBinding = entries[i].dstBinding;
		descriptor.m_BlobOffset = entries[i].offset;

		if (i >= pCreateInfo->descriptorUpdateEntryCount)
		{
			descriptor.m_ArrayStride = std::max(16u, FormatToSize(pass_descriptors[i - pCreateInfo->descriptorUpdateEntryCount].m_ElementFormat));
		}
	}

	// Final blob size
	handle->m_BlobSize = offset;

	// Proxy entries and layout
	auto create_info = *pCreateInfo;
	create_info.descriptorSetLayout = set_layout->m_SetLayout;
	create_info.descriptorUpdateEntryCount = pCreateInfo->descriptorUpdateEntryCount + pass_descriptor_count;
	create_info.pDescriptorUpdateEntries = entries;

	// Create template
	VkResult result = table->m_CreateDescriptorUpdateTemplate(device, &create_info, pAllocator, &handle->m_Template);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// OK
	*pDescriptorUpdateTemplate = reinterpret_cast<VkDescriptorUpdateTemplate>(handle);
	return VK_SUCCESS;
}

void VKAPI_CALL UpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void * pData)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Unwrap set
	auto set_handle = reinterpret_cast<HDescriptorSet*>(descriptorSet);
	set_handle->m_CommitHash = 0;

	// Get handle
	auto handle = reinterpret_cast<HDescriptorUpdateTemplate*>(descriptorUpdateTemplate);

	// Copy blob and copy top descriptor data
	auto blob = ALLOCA_ARRAY(uint8_t, handle->m_BlobSize);
	std::memcpy(blob, pData, handle->m_TopBlobSize);

	// Active report?
	uint32_t feature_set = 0x0;
	if (device_state->m_ActiveReport)
	{
		feature_set = device_state->m_ActiveReport->m_BeginInfo.m_Features;
	}

	// Perform tracking
	for (const SDescriptor& template_descriptor : handle->m_Descriptors)
	{
		STrackedWrite tracked_write;
		tracked_write.m_DstBinding = template_descriptor.m_DstBinding;
		tracked_write.m_DescriptorType = template_descriptor.m_DescriptorType;
		tracked_write.m_DescriptorCount = template_descriptor.m_DescriptorCount;
		tracked_write.m_DstArrayElement = 0;

		// Translate blob'ed descriptor
		switch (template_descriptor.m_DescriptorType)
		{
		default:
			// Unsupported feature used by pass
			break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			tracked_write.m_ImageInfo = *reinterpret_cast<VkDescriptorImageInfo*>(&blob[template_descriptor.m_BlobOffset]);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			tracked_write.m_TexelBufferView = *reinterpret_cast<VkBufferView*>(&blob[template_descriptor.m_BlobOffset]);
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			tracked_write.m_BufferInfo = *reinterpret_cast<VkDescriptorBufferInfo*>(&blob[template_descriptor.m_BlobOffset]);
			break;
		}

		if (template_descriptor.m_DstBinding < set_handle->m_SetLayout->m_TopBinding)
		{
			set_handle->m_TrackedWrites[template_descriptor.m_DstBinding] = tracked_write;

            // Hash the write
            CombineHash(set_handle->m_CommitHash, ComputeCRC64Buffer(tracked_write));
		}
	}

	// Update pass descriptor data
	{
		std::lock_guard<std::mutex> guard(device_state->m_ResourceLock);
		device_state->m_DiagnosticRegistry->UpdateDescriptors(set_handle, true, feature_set, &handle->m_Descriptors[0], &handle->m_Descriptors[handle->m_TopCount], handle->m_TopCount, blob);
	}

	// Pass down callchain
	table->m_UpdateDescriptorSetWithTemplate(device, set_handle->m_Set, handle->m_Template, blob);

	// Considered valid at this point
	set_handle->m_Valid = true;

	// Increment commit
	set_handle->m_CommitIndex++;
}

void VKAPI_CALL CmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
	const VkWriteDescriptorSet* pDescriptorWrites)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, reinterpret_cast<HPipelineLayout*>(layout)->m_Layout, set, descriptorWriteCount, pDescriptorWrites);
}

void VKAPI_CALL CmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set,
	const void* pData)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(commandBuffer));

	// Pass down callchain
	table->m_CmdPushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, reinterpret_cast<HPipelineLayout*>(layout)->m_Layout, set, pData);
}

VkResult InstrumentDescriptors(VkDevice device, VkGPUValidationReportAVA report)
{
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	// Lock everything!
	std::lock_guard<std::mutex> guard(state->m_ResourceLock);

	// Recompile all pools
	for (HDescriptorPool* pool : state->m_ResourceDescriptorPoolSwaptable)
	{
		// While descriptor pools usages are serial, this extension may concurrently iterate them
		std::lock_guard<std::mutex> pool_lock(pool->m_InternalLock);

		// Recompile currently tracked sets
		for (HDescriptorSet* set : pool->m_Sets)
		{
			// Only recompile valid sets
			if (!set->m_Valid)
				continue;

			InstrumentedLiveSet(device, set, report->m_BeginInfo.m_Features, static_cast<uint32_t>(set->m_TrackedWrites.size()), set->m_TrackedWrites.data());
		}
	}

	// All good
	return VK_SUCCESS;
}

VkGPUValidationObjectInfoAVA GetDescriptorObjectInfo(DeviceStateTable* state, const STrackedWrite& descriptor)
{
    std::lock_guard<std::mutex> resource_guard(state->m_ResourceLock);

    // Get the underlying key
    void* resource_key = nullptr;
    switch (descriptor.m_DescriptorType)
    {
        default:
            // Unsupported feature used by pass
            break;

        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            resource_key = state->m_ResourceImageViewSources[descriptor.m_ImageInfo.imageView].image;
            break;

        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            resource_key = state->m_ResourceBufferViewSources[descriptor.m_TexelBufferView].buffer;
            break;

        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            resource_key = descriptor.m_BufferInfo.buffer;
            break;
    }

    // Assign handle info
    VkGPUValidationObjectInfoAVA info{};
    info.m_Object = reinterpret_cast<VkGPUValidationObjectAVA>(resource_key);

    // May not have been mapped
    auto debug_it = state->m_ResourceDebugNames.find(resource_key);
    if (debug_it != state->m_ResourceDebugNames.end())
    {
        info.m_Name = debug_it->second.c_str();
    }

    return info;
}