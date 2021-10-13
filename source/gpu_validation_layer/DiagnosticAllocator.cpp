#include <DiagnosticAllocator.h>
#include <StateTables.h>
#include <SPIRV/DiagnosticPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>

#ifndef DIAGNOSTIC_ALLOCATOR_DEBUG_CHECK
#	define DIAGNOSTIC_ALLOCATOR_DEBUG_CHECK 0
#endif

// Debugging values
static constexpr auto kDebugDefault = 42;
static constexpr auto kDebugMoved = 56;

DiagnosticAllocator::DiagnosticAllocator()
{
	/* */
}

VkResult DiagnosticAllocator::Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, const VkAllocationCallbacks* allocator, DiagnosticRegistry* registry)
{
	m_Device = device;
	m_PhysicalDevice = physicalDevice;
	m_Allocator = allocator;
	m_Registry = registry;
	m_DeviceTable = DeviceDispatchTable::Get(GetKey(device));
	m_DeviceState = DeviceStateTable::Get(GetKey(device));
	m_InstanceTable = InstanceDispatchTable::Get(GetKey(instance));

	// Start thread
	m_Thread = std::thread([](DiagnosticAllocator* cache) { cache->ThreadEntry_MessageFiltering(); }, this);

	// Attempt to create layout
	VkResult result = CreateLayout();
	if (result != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

void DiagnosticAllocator::Release()
{
	{
		std::unique_lock<std::mutex> unique(m_PendingMutex);
		m_ThreadExitFlag = true;
		m_ThreadWakeVar.notify_all();
	}

	// Wait for all worker
	if (m_Thread.joinable())
	{
		m_Thread.join();
	}

	// Release all pending allocations
	for (SDiagnosticAllocation* pending : m_PendingAllocations)
	{
		DestroyAllocation(pending);
	}

	// Release all mirror allocations
	for (SMirrorAllocation* mirror : m_ThreadDiagnosticMirrorPool)
	{
		Free(mirror->m_HeapAllocation.m_Binding.m_Heap, mirror->m_HeapAllocation.m_Binding.m_AllocationIt);
	}

	// Release all heaps
	ReleaseHeapType(m_MirrorHeap);
	ReleaseHeapType(m_DeviceHeap);
	ReleaseHeapType(m_DescriptorHeap);

	// Destroy all pools
	for (VkDescriptorPool pool : m_DescriptorPools)
	{
		m_DeviceTable->m_DestroyDescriptorPool(m_Device, pool, nullptr);
	}

	// Destroy set layout
	m_DeviceTable->m_DestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);

	// Destroy pipeline layout
	m_DeviceTable->m_DestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
}

void DiagnosticAllocator::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	DeviceDispatchTable::Get(GetKey(m_Device))->m_GetPhysicalDeviceProperties2(m_PhysicalDevice, &properties);

	// Register diagnostic pass
	optimizer->RegisterPass(SPIRV::CreatePassToken<SPIRV::DiagnosticPass>(state, properties));
}

void DiagnosticAllocator::StartMessageFiltering(SDiagnosticAllocation* pending)
{
	// May not have an allocation, i.e. already filtered
	if (!pending->m_MirrorAllocation)
		return;

	if (!pending->m_MirrorAllocation->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_IsHostCoherent)
	{
		VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		range.memory = pending->m_MirrorAllocation->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_DeviceMemory;
		range.offset = pending->m_MirrorAllocation->m_HeapAllocation.m_Binding.m_AllocationIt->m_Offset;
		range.size = AlignUpper(pending->m_MirrorAllocation->m_HeapAllocation.m_HeapSpan, m_DeviceState->m_PhysicalDeviceProperties.properties.limits.nonCoherentAtomSize);

		VkResult result = m_DeviceTable->m_InvalidateMappedMemoryRanges(m_Device, 1, &range);
		if (result != VK_SUCCESS)
		{
			return;
		}
	}

	// Diagnostic data is always first
	// ! The data is mirrored from the device memory, any modification is host visible only
	auto diagnostic_data = reinterpret_cast<SDiagnosticData*>(pending->m_MirrorAllocation->m_HeapAllocation.m_Binding.m_MappedData);

	// Message cap?
	if (diagnostic_data->m_MessageCount > diagnostic_data->m_MessageLimit)
	{
		if (m_MessageCounter.Next(15) && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
		{
			char buffer[512];
			FormatBuffer(buffer, "Command list generated a total of %i validation messages but is capped to %i", diagnostic_data->m_MessageCount, diagnostic_data->m_MessageLimit);
			m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
		}

		// Account for overflow in tag tracking
		// Apply growth factor for future allocation requests
		if (pending->m_ActiveTag)
		{
			m_TagMessageCounters[pending->m_ActiveTag] = static_cast<uint32_t>(diagnostic_data->m_MessageCount * m_GrowthFactor);
		}
	}

	// Counter may not represent actual message count
	pending->m_LastMessageCount = std::min(diagnostic_data->m_MessageCount, diagnostic_data->m_MessageLimit);

	{
		// Smooth out the average count
		m_AverageMessageCount = static_cast<uint32_t>(m_AverageMessageCount * m_AverageMessageWeight + pending->m_LastMessageCount * (1.f - m_AverageMessageWeight));

		// Deduce if this allocation is a viable sync point
		pending->m_IsTransferSyncPoint = ((pending->m_LastMessageCount / static_cast<float>(m_AverageMessageCount)) > m_TransferSyncPointThreshold);
	}

	// Collect all storage buffers
	/*m_ImmediateStorageLookup.resize(m_Registry->GetAllocatedStorageUIDs());
	for (uint32_t j = 0; j < m_LayoutStorageInfo.size(); j++)
	{
		m_ImmediateStorageLookup[m_LayoutStorageInfo[j].m_UID] = &mapped_data[pending->m_Storages[j].m_HeapOffset - pending->m_HeapAllocation.m_Binding.m_Offset];
	}*/

	// Push latent count
	if (pending->m_ActiveTag)
	{
		STagCounterBuffer& latent_tag_buffer = m_LatentTagMessageCounter[pending->m_ActiveTag];
		latent_tag_buffer.m_Buffer[latent_tag_buffer.m_Index = ((latent_tag_buffer.m_Index + 1) % STagCounterBuffer::kCount)] = pending->m_LastMessageCount;
	}

	// Push to thread if needed
	if (pending->m_LastMessageCount > 0)
	{
		// Check for corruption
#if DIAGNOSTIC_ALLOCATOR_DEBUG_CHECK
		if (diagnostic_data->m_Debug != kDebugDefault && diagnostic_data->m_Debug != kDebugMoved)
		{
			throw std::exception();
		}
#endif

		if (m_DeviceState->m_ActiveReport)
		{
			m_DeviceState->m_ActiveReport->m_ExportedMessages += pending->m_LastMessageCount;

			// Track latent overshoots and undershoots
			if (m_DeviceTable->m_CreateInfoAVA.m_LatentTransfers)
			{
				if (diagnostic_data->m_TransferredMessageCount < pending->m_LastMessageCount)
				{
					m_DeviceState->m_ActiveReport->m_LatentUndershoots += (pending->m_LastMessageCount - diagnostic_data->m_TransferredMessageCount);
				}

				if (diagnostic_data->m_TransferredMessageCount > pending->m_LastMessageCount)
				{
					m_DeviceState->m_ActiveReport->m_LatentOvershoots += (diagnostic_data->m_TransferredMessageCount - pending->m_LastMessageCount);
				}
			}
		}

		// Modify message count for registry
		diagnostic_data->m_MessageCount = std::min(diagnostic_data->m_TransferredMessageCount, pending->m_LastMessageCount);

		if (m_DeviceState->m_ActiveReport)
		{
			m_DeviceState->m_ActiveReport->m_FilteredMessages += diagnostic_data->m_MessageCount;
		}

		// Push to thread
		SPendingDiagnosticAllocation proxy;
		proxy.m_Allocation = pending->m_MirrorAllocation;
		proxy.m_ThrottleAge = 0;
		{
			std::lock_guard<std::mutex> guard(m_PendingMutex);
			m_PendingDiagnosticData.push_back(proxy);
		}

		// Wake
		m_ThreadWakeVar.notify_one();

		// Considered invalid
		pending->m_MirrorAllocation = nullptr;
	}
}

void DiagnosticAllocator::UpdateHeader(VkCommandBuffer cmd_buffer, SDiagnosticAllocation* allocation)
{
	// Prepare header
	SDiagnosticData data;
	data.m_MessageCount = 0;
	data.m_MessageLimit = allocation->m_MessageLimit;
	data.m_Debug = allocation->m_DebugData;
	data.m_TransferredMessageCount = 0;

	// Update header
	m_DeviceTable->m_CmdUpdateBuffer(cmd_buffer, allocation->m_DeviceAllocation.m_HeapBuffer, 0, sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData), &data);

	VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.size = VK_WHOLE_SIZE;
	barrier.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	// Wait for previous update to finish before starting validation
	m_DeviceTable->m_CmdPipelineBarrier(
		cmd_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, nullptr, 1, &barrier, 0, nullptr
	);
}

VkResult DiagnosticAllocator::PopMirrorAllocation(uint32_t message_limit, SMirrorAllocation** out)
{
	VkResult result;

	// Check pool
	{
		m_ThreadDiagnosticMirrorPoolMutex.lock();

		for (size_t i = 0; i < m_ThreadDiagnosticMirrorPool.size(); i++)
		{
			// Check if it can faciliate and apply viability threshold to avoid worst case limit scenario
			if (m_ThreadDiagnosticMirrorPool[i]->m_MessageLimit < message_limit || 
				(m_ThreadDiagnosticMirrorPool[i]->m_MessageLimit / static_cast<float>(message_limit)) > m_AllocationViabilityLimitThreshold)
				continue;

			SMirrorAllocation* alloc = m_ThreadDiagnosticMirrorPool[i];

			m_ThreadDiagnosticMirrorPool.erase(m_ThreadDiagnosticMirrorPool.begin() + i);
			m_ThreadDiagnosticMirrorPoolMutex.unlock();

			// Reset count
			// ! Header range should be valid here
			auto diagnostic_data = reinterpret_cast<SDiagnosticData*>(alloc->m_HeapAllocation.m_Binding.m_MappedData);
			diagnostic_data->m_MessageCount = 0;
			diagnostic_data->m_TransferredMessageCount = 0;
			diagnostic_data->m_Debug = 0;

			// Update host header
			if (!alloc->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_IsHostCoherent)
			{
				VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
				range.memory = alloc->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_DeviceMemory;
				range.offset = alloc->m_HeapAllocation.m_Binding.m_AllocationIt->m_Offset;
				range.size = std::max(m_DeviceState->m_PhysicalDeviceProperties.properties.limits.nonCoherentAtomSize, sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData));
				if ((result = m_DeviceTable->m_FlushMappedMemoryRanges(m_Device, 1, &range)) != VK_SUCCESS)
				{
					return result;
				}
			}

			*out = alloc;
			return VK_SUCCESS;
		}

		m_ThreadDiagnosticMirrorPoolMutex.unlock();
	}

	// Create new allocation
	auto alloc = new SMirrorAllocation();
	alloc->m_MessageLimit = message_limit;

	// Deduce size
	size_t allocation_size = sizeof(SDiagnosticData) + sizeof(SDiagnosticMessageData) * (message_limit - 1);

	// Add non coherency atom limit
	allocation_size += m_DeviceState->m_PhysicalDeviceProperties.properties.limits.nonCoherentAtomSize;

	// Host visible memory can assume only a single queue is accessing it's data in a coherent state
	VkBufferCreateInfo host_buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	host_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	host_buffer_info.size = allocation_size;
	if ((result = m_DeviceTable->m_CreateBuffer(m_Device, &host_buffer_info, m_Allocator, &alloc->m_HeapAllocation.m_HeapBuffer)) != VK_SUCCESS)
	{
		return result;
	}

	// Attempt to bind from any available heap
	if (!AllocateOrBind(m_MirrorHeap, host_buffer_info.size, &alloc->m_HeapAllocation))
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	// Reset count
	// ! Header range should be valid here
	auto diagnostic_data = reinterpret_cast<SDiagnosticData*>(alloc->m_HeapAllocation.m_Binding.m_MappedData);
	diagnostic_data->m_MessageCount = 0;
	diagnostic_data->m_MessageLimit = message_limit;
	diagnostic_data->m_TransferredMessageCount = 0;
	diagnostic_data->m_Debug = 0;

	// Update host header
	if (!alloc->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_IsHostCoherent)
	{
		VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		range.memory = alloc->m_HeapAllocation.m_Binding.m_Heap->m_Memory.m_DeviceMemory;
		range.offset = alloc->m_HeapAllocation.m_Binding.m_AllocationIt->m_Offset;
		range.size = std::max(m_DeviceState->m_PhysicalDeviceProperties.properties.limits.nonCoherentAtomSize, sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData));
		if ((result = m_DeviceTable->m_FlushMappedMemoryRanges(m_Device, 1, &range)) != VK_SUCCESS)
		{
			return result;
		}
	}

	// Ok
	*out = alloc;
	return VK_SUCCESS;
}

VkResult DiagnosticAllocator::RebindAllocationDeviceMemory(SDiagnosticAllocation* allocation)
{
	VkResult result;

	// Perform rebinding
	size_t rebind_Working_set = RebindHeapAllocation(allocation->m_DeviceAllocation.m_Binding.m_Heap, allocation->m_DeviceAllocation.m_Binding.m_AllocationIt);

	// Free states dependent on previous buffer
	{
		m_DeviceTable->m_DestroyBuffer(m_Device, allocation->m_DeviceAllocation.m_HeapBuffer, m_Allocator);

		std::lock_guard<std::mutex> guard(m_DescriptorLock);
		if ((result = m_DeviceTable->m_FreeDescriptorSet(m_Device, allocation->m_DescriptorPool, 1, &allocation->m_DescriptorSet)) != VK_SUCCESS)
		{
			return result;
		}
	}

	// Recreate buffer
	{
		// Use previous creation info
		if ((result = m_DeviceTable->m_CreateBuffer(m_Device, &allocation->m_DeviceAllocation.m_CreateInfo, m_Allocator, &allocation->m_DeviceAllocation.m_HeapBuffer)) != VK_SUCCESS)
		{
			return result;
		}

		// Bind vulkan memory
		// Allocation iterator will contain new offsets
		if (!AllocateOrBind(m_DeviceHeap, allocation->m_DeviceAllocation.m_HeapSpan, &allocation->m_DeviceAllocation))
		{
			return VK_ERROR_OUT_OF_DEVICE_MEMORY;
		}
	}

	// Recreate descriptor data
	{
		// Proxy the descriptor
		allocation->m_BufferDescriptor.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;

		// Note: Does not destroy existing states
		if ((result = CreateAllocationDescriptors(allocation)) != VK_SUCCESS)
		{
			return result;
		}
	}

	// Diagnostic
	if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[256];
		FormatBuffer(buffer, "Defragmentation completed for empty span of %i bytes", rebind_Working_set);
		m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	allocation->m_DebugData = kDebugMoved;
	return VK_SUCCESS;
}

VkResult DiagnosticAllocator::CreateAllocationDescriptors(SDiagnosticAllocation * allocation)
{
	VkResult result;

	{
		// Allocation info
		VkDescriptorSetAllocateInfo set_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		set_info.descriptorSetCount = 1;
		set_info.pSetLayouts = &m_SetLayout;

		// Attempt to allocate
		SDiagnosticDescriptorBinding binding;
		if ((result = AllocateDescriptorSet(set_info, &binding)) != VK_SUCCESS)
		{
			return result;
		}

		// Track pool
		allocation->m_DescriptorPool = binding.m_Pool;
		allocation->m_DescriptorSet = binding.m_Set;
	}

	// Update descriptor set
	{
		// Describe bindings
		auto writes = (VkWriteDescriptorSet*)alloca(sizeof(VkWriteDescriptorSet) * (m_LayoutStorageInfo.size() + 1));
		for (uint32_t j = 0; j < m_LayoutStorageInfo.size(); j++)
		{
			auto& write = writes[j] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.dstBinding = m_LayoutStorageInfo[j].m_UID;
			write.pBufferInfo = &allocation->m_Storages[j].m_Descriptor;
			write.dstSet = allocation->m_DescriptorSet;
		}

		// Base descriptor
		{
			auto& write = writes[m_LayoutStorageInfo.size()] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.dstBinding = 0;
			write.pBufferInfo = &allocation->m_BufferDescriptor;
			write.dstSet = allocation->m_DescriptorSet;
		}

		// Note: No return code, that's nice
		m_DeviceTable->m_UpdateDescriptorSets(m_Device, (uint32_t)m_LayoutStorageInfo.size() + 1, writes, 0, nullptr);
	}

	return VK_SUCCESS;
}

uint32_t DiagnosticAllocator::WaitForPendingAllocations()
{
	std::lock_guard<std::mutex> guard(m_AllocationMutex);

	// Does not remove the pending allocations
	for (SDiagnosticAllocation* pending : m_PendingAllocations)
	{
		// Busy wait
		while (pending->GetUnsafeFence() && !pending->IsDone(m_Device, m_DeviceTable));

		// Free fence if last reference
		SDiagnosticFence* fence = pending->GetUnsafeFence();
		if (fence && --fence->m_ReferenceCount == 0)
		{
			m_DeviceTable->m_ResetFences(m_Device, 1, &fence->m_Fence);
			m_FreeFences.push_back(fence);
		}

		// Move data for async processing
		StartMessageFiltering(pending);

		// No longer associated
		pending->SetFence(nullptr);
	}

	return static_cast<uint32_t>(m_PendingAllocations.size());
}

SDiagnosticAllocation* DiagnosticAllocator::PopAllocation(VkCommandBuffer cmd_buffer, uint64_t tag)
{
	VkResult result;

	uint32_t requested_message_limit = 0;
	{
		m_AllocationMutex.lock();

		// Latent message count tracked per tag
		uint32_t latent_tag_counter = 0;

		// Account for in-tag growth
		requested_message_limit = m_DeviceTable->m_CreateInfoAVA.m_CommandBufferMessageCountDefault;
		if (tag)
		{
			uint32_t tag_counter = m_TagMessageCounters[tag];

			requested_message_limit = std::min(m_DeviceTable->m_CreateInfoAVA.m_CommandBufferMessageCountLimit, std::max(m_DeviceTable->m_CreateInfoAVA.m_CommandBufferMessageCountDefault, tag_counter));

			// Get the maximum latent value for estimation
			STagCounterBuffer& latent_buffer = m_LatentTagMessageCounter[tag];
			for (uint32_t value : latent_buffer.m_Buffer)
			{
				latent_tag_counter = std::max(latent_tag_counter, value);
			}
		}

		// Iterate pending allocations
		// ! More efficient than on the async worker thread
		for (size_t i = 0; i < m_PendingAllocations.size(); i++)
		{
			SDiagnosticAllocation* pending = m_PendingAllocations[i];

			// Must be able to faciliate limits, and apply size threshold
			// This is needed as without this the worst case scenario is that all allocations assume worst case message limits
			if (pending->m_MessageLimit < requested_message_limit || (pending->m_MessageLimit / static_cast<float>(requested_message_limit)) > m_AllocationViabilityLimitThreshold)
				continue;

			// Check grouped fence
			if (pending->IsDone(m_Device, m_DeviceTable))
			{
				SDiagnosticFence* fence = pending->GetUnsafeFence();

				// Free fence if last reference
				if (fence && --fence->m_ReferenceCount == 0)
				{
					m_DeviceTable->m_ResetFences(m_Device, 1, &fence->m_Fence);
					m_FreeFences.push_back(fence);
				}

				// Remove from host
				m_PendingAllocations.erase(m_PendingAllocations.begin() + i);

				// Move data for async processing
				StartMessageFiltering(pending);
				m_AllocationMutex.unlock();

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
				bool removed_live_range = false;

				// Remove this range as active
				{
					std::lock_guard<std::mutex> guard(m_HeapMutex);

					auto range = std::make_pair(
						pending->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset,
						pending->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset + pending->m_DeviceAllocation.m_HeapSpan
					);

					auto& ranges = pending->m_DeviceAllocation.m_Binding.m_Heap->m_LiveGPURanges;
					for (auto it = ranges.begin(); it != ranges.end(); it++)
					{
						if (it->m_Alloc == pending->m_DeviceAllocation.m_Binding.m_AllocationIt)
						{
							if (it->m_MemoryRange != range)
							{
								throw std::exception();
							}

							ranges.erase(it);
							removed_live_range = true;
							break;
						}
					}
				}
#endif

				// Device memory rebind request?
				if (pending->m_DeviceAllocation.m_Binding.m_AllocationIt->m_RebindRequest.m_Requested && (result = RebindAllocationDeviceMemory(pending)) != VK_SUCCESS)
				{
					return nullptr;
				}

				// Reset device data
				UpdateHeader(cmd_buffer, pending);

				// Pop a new mirror allocation if needed
				if (!pending->m_MirrorAllocation && (result = PopMirrorAllocation(pending->m_MessageLimit, &pending->m_MirrorAllocation)) != VK_SUCCESS)
				{
					return nullptr;
				}

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
				// Live range check
				{
					std::lock_guard<std::mutex> guard(m_HeapMutex);

					auto range = std::make_pair(
						pending->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset,
						pending->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset + pending->m_DeviceAllocation.m_HeapSpan
					);

					// Live range check
					pending->m_DeviceAllocation.m_Binding.m_Heap->CheckGPURangeOverlap(range.first, range.second);
					pending->m_DeviceAllocation.m_Binding.m_Heap->m_LiveGPURanges.push_back({ range, pending->m_DeviceAllocation.m_Binding.m_AllocationIt });
				}
#endif

				// The tag may not be reliable!
				latent_tag_counter = std::min(latent_tag_counter, pending->m_MessageLimit);

				// Fence no longer associated
				pending->Reset(tag, latent_tag_counter);
				return pending;
			}
		}

		m_AllocationMutex.unlock();
	}

	// Diagnostic information
	if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[256];
		FormatBuffer(buffer, "Allocated a new message stream with message limit %i", requested_message_limit);
		m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	// Create allocation
	auto allocation = new SDiagnosticAllocation();
	allocation->m_MessageLimit = requested_message_limit;
	allocation->m_IsTransferSyncPoint = true;
	allocation->m_LastMessageCount = 0;
	allocation->m_DebugData = kDebugDefault;
	allocation->Reset(tag, 0);

	// May not have async queue
	if (m_DeviceState->m_TransferQueue)
	{
		// Create transfer semaphore
		VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		if ((result = m_DeviceTable->m_CreateSemaphore(m_Device, &semaphore_info, m_Allocator, &allocation->m_TransferSignalSemaphore)) != VK_SUCCESS)
		{
			return nullptr;
		}

		// Transfer allocations are serial
		std::lock_guard<std::mutex> guard(m_DeviceState->m_TransferPoolMutex);

		// Allocate command buffer
		VkCommandBufferAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandBufferCount = 1;
		alloc_info.commandPool = m_DeviceState->m_TransferPool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if ((result = m_DeviceTable->m_AllocateCommandBuffers(m_Device, &alloc_info, &allocation->m_TransferCmdBuffer)) != VK_SUCCESS)
		{
			return nullptr;
		}

		// Patch the internal dispatch tables
		PatchDispatchTable(m_InstanceTable, m_Device, allocation->m_TransferSignalSemaphore);
		PatchDispatchTable(m_InstanceTable, m_Device, allocation->m_TransferCmdBuffer);
	}

	// Attempt to create device heap buffer
	{
		// Device local memory needs to assume worst case scenario queue ownership
		VkBufferCreateInfo& device_buffer_info = allocation->m_DeviceAllocation.m_CreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		device_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		device_buffer_info.size = sizeof(SDiagnosticData) + sizeof(SDiagnosticMessageData) * (requested_message_limit - 1);
		device_buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(m_DeviceState->m_QueueFamilyIndices.size());
		device_buffer_info.pQueueFamilyIndices = m_DeviceState->m_QueueFamilyIndices.data();
		if ((result = m_DeviceTable->m_CreateBuffer(m_Device, &device_buffer_info, m_Allocator, &allocation->m_DeviceAllocation.m_HeapBuffer)) != VK_SUCCESS)
		{
			return nullptr;
		}

		// Attempt to bind from any available heap
		if (!AllocateOrBind(m_DeviceHeap, device_buffer_info.size, &allocation->m_DeviceAllocation))
			return nullptr;

		// Device Descriptor
		allocation->m_BufferDescriptor.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;
		allocation->m_BufferDescriptor.range = allocation->m_DeviceAllocation.m_HeapSpan;
		allocation->m_BufferDescriptor.offset = 0;
	}

	// Pop a new mirror allocation
	if ((result = PopMirrorAllocation(requested_message_limit, &allocation->m_MirrorAllocation)) != VK_SUCCESS)
	{
		return nullptr;
	}

	// Prepare allocation
	UpdateHeader(cmd_buffer, allocation);

	// Create storage buffers
	/*allocation->m_Storages.resize(m_LayoutStorageInfo.size());
	for (uint32_t j = 0; j < m_LayoutStorageInfo.size(); j++)
	{
		auto& storage = allocation->m_Storages[j];

		// Attempt to create buffer
		device_buffer_info.size = 0;
		if ((result = m_DeviceTable->m_CreateBuffer(m_Device, &device_buffer_info, m_Allocator, nullptr)) != VK_SUCCESS)
		{
			return nullptr;
		}

		// Bind
		// storage.m_HeapOffset = AllocatedBind(storage.m_Buffer, device_buffer_info.size, nullptr);

		// Descriptor
		storage.m_Descriptor.buffer = storage.m_Buffer;
		storage.m_Descriptor.offset = 0;
		storage.m_Descriptor.range = VK_WHOLE_SIZE;
	}*/

	// Attempt to create descriptors
	if ((result = CreateAllocationDescriptors(allocation)) != VK_SUCCESS)
	{
		return nullptr;
	}

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
	// Live range check
	{
		std::lock_guard<std::mutex> guard(m_HeapMutex);

		auto range = std::make_pair(
			allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset,
			allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset + allocation->m_DeviceAllocation.m_HeapSpan
		);

		// Live range check
		allocation->m_DeviceAllocation.m_Binding.m_Heap->CheckGPURangeOverlap(range.first, range.second);
		allocation->m_DeviceAllocation.m_Binding.m_Heap->m_LiveGPURanges.push_back({ range, allocation->m_DeviceAllocation.m_Binding.m_AllocationIt });
	}
#endif

	return allocation;
}

void DiagnosticAllocator::TransferInplaceAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation * allocation)
{
	SAllocationTransfer transfer = allocation->GetTransfer(m_DeviceTable->m_CreateInfoAVA.m_LatentTransfers);

	VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.size = transfer.m_ByteSpan;
	barrier.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	// Wait for writes
	m_DeviceTable->m_CmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	// Push latent header
	m_DeviceTable->m_CmdUpdateBuffer(cmd_buffer, allocation->m_DeviceAllocation.m_HeapBuffer, sizeof(uint32_t) * 2, sizeof(uint32_t), &transfer.m_MessageCount);

	// Wait for latent header
	barrier.size = sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData);
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	m_DeviceTable->m_CmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	// Copy all data to mirrored buffer
	VkBufferCopy region{};
	region.size = transfer.m_ByteSpan;
	m_DeviceTable->m_CmdCopyBuffer(cmd_buffer, allocation->m_DeviceAllocation.m_HeapBuffer, allocation->m_MirrorAllocation->m_HeapAllocation.m_HeapBuffer, 1, &region);
}

void DiagnosticAllocator::BeginTransferAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation * allocation)
{
	SAllocationTransfer transfer = allocation->GetTransfer(m_DeviceTable->m_CreateInfoAVA.m_LatentTransfers);

	// Get originating family index
	{
		std::lock_guard<std::mutex> guard(m_DeviceState->m_CommandFamilyIndexMutex);
		allocation->m_SourceFamilyIndex = m_DeviceState->m_CommandBufferFamilyIndices[cmd_buffer];
	}

	VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.size = transfer.m_ByteSpan;
	barrier.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;
	barrier.srcQueueFamilyIndex = allocation->m_SourceFamilyIndex;
	barrier.dstQueueFamilyIndex = m_DeviceState->m_DedicatedTransferQueueFamily;

	// Ownership release
	m_DeviceTable->m_CmdPipelineBarrier(
		cmd_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, nullptr, 1, &barrier, 0, nullptr
	);
}

void DiagnosticAllocator::EndTransferAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation * allocation)
{
	SAllocationTransfer transfer = allocation->GetTransfer(m_DeviceTable->m_CreateInfoAVA.m_LatentTransfers);

	VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.size = transfer.m_ByteSpan;
	barrier.buffer = allocation->m_DeviceAllocation.m_HeapBuffer;
	barrier.srcQueueFamilyIndex = allocation->m_SourceFamilyIndex;
	barrier.dstQueueFamilyIndex = m_DeviceState->m_DedicatedTransferQueueFamily;

	// Ownership acquisition
	m_DeviceTable->m_CmdPipelineBarrier(
		cmd_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 0, nullptr, 1, &barrier, 0, nullptr
	);

	// Push latent header
	m_DeviceTable->m_CmdUpdateBuffer(cmd_buffer, allocation->m_DeviceAllocation.m_HeapBuffer, sizeof(uint32_t) * 2, sizeof(uint32_t), &transfer.m_MessageCount);

	// Wait for latent header
	barrier.size = sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData);
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	m_DeviceTable->m_CmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	// Copy all data to mirrored buffer
	VkBufferCopy region{};
	region.size = transfer.m_ByteSpan;
	m_DeviceTable->m_CmdCopyBuffer(cmd_buffer, allocation->m_DeviceAllocation.m_HeapBuffer, allocation->m_MirrorAllocation->m_HeapAllocation.m_HeapBuffer, 1, &region);
}

void DiagnosticAllocator::PushAllocation(SDiagnosticAllocation * allocation)
{
	// Push to filtering thread
	std::lock_guard<std::mutex> guard(m_AllocationMutex);
	m_PendingAllocations.push_back(allocation);
}

void DiagnosticAllocator::DestroyAllocation(SDiagnosticAllocation * allocation)
{
	// Diagnostic information
	if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[256];
		FormatBuffer(buffer, "Destroying a message stream with message limit %i", allocation->m_MessageLimit);
		m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	// Free the device allocation
	Free(allocation->m_DeviceAllocation.m_Binding.m_Heap, allocation->m_DeviceAllocation.m_Binding.m_AllocationIt);

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK

	// Remove this range as active
	{
		bool removed = false;
		std::lock_guard<std::mutex> guard(m_HeapMutex);

		auto range = std::make_pair(
			allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset,
			allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset + allocation->m_DeviceAllocation.m_HeapSpan
		);

		auto& ranges = allocation->m_DeviceAllocation.m_Binding.m_Heap->m_LiveGPURanges;
		for (auto it = ranges.begin(); it != ranges.end(); it++)
		{
			if (it->m_Alloc == allocation->m_DeviceAllocation.m_Binding.m_AllocationIt)
			{
				if (it->m_MemoryRange != range)
				{
					throw std::exception();
				}

				ranges.erase(it);
				removed = true;
				break;
			}
		}

		if (allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_RebindRequest.m_Requested)
		{
			if (!allocation->m_DeviceAllocation.m_Binding.m_Heap->m_AllocationsOffsets.count(allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_RebindRequest.m_RebindOffset)) throw std::exception();
			allocation->m_DeviceAllocation.m_Binding.m_Heap->m_AllocationsOffsets.erase(allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_RebindRequest.m_RebindOffset);
		}
		else
		{
			if (!allocation->m_DeviceAllocation.m_Binding.m_Heap->m_AllocationsOffsets.count(allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset)) throw std::exception();
			allocation->m_DeviceAllocation.m_Binding.m_Heap->m_AllocationsOffsets.erase(allocation->m_DeviceAllocation.m_Binding.m_AllocationIt->m_Offset);
		}

		if (!removed) throw std::exception();
	}
#endif

	// Free transfer resources
	if (m_DeviceState->m_TransferQueue)
	{
		m_DeviceTable->m_DestroySemaphore(m_Device, allocation->m_TransferSignalSemaphore, nullptr);
		m_DeviceTable->m_FreeCommandBuffers(m_Device, m_DeviceState->m_TransferPool, 1, &allocation->m_TransferCmdBuffer);
	}

	// Outstanding mirror allocation?
	if (allocation->m_MirrorAllocation)
	{
		std::lock_guard<std::mutex> unique(m_ThreadDiagnosticMirrorPoolMutex);
		m_ThreadDiagnosticMirrorPool.push_back(allocation->m_MirrorAllocation);
	}

	// TODO: Destroy states
	delete allocation;
}

VkResult DiagnosticAllocator::AllocateDescriptorBinding(uint64_t alignment, uint64_t size, SDiagnosticHeapBinding * out)
{
	return Allocate(m_DescriptorHeap, alignment, size, out) ? VK_SUCCESS : VK_ERROR_OUT_OF_HOST_MEMORY;
}

void DiagnosticAllocator::FreeDescriptorBinding(const SDiagnosticHeapBinding & binding)
{
	Free(binding.m_Heap, binding.m_AllocationIt);
}

VkResult DiagnosticAllocator::AllocateDeviceBinding(uint64_t alignment, uint64_t size, SDiagnosticHeapBinding * out)
{
	return Allocate(m_DeviceHeap, alignment, size, out) ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

void DiagnosticAllocator::FreeDeviceBinding(const SDiagnosticHeapBinding & binding)
{
	Free(binding.m_Heap, binding.m_AllocationIt);
}

VkResult DiagnosticAllocator::AllocateDescriptorSet(const VkDescriptorSetAllocateInfo & info, SDiagnosticDescriptorBinding* out)
{
	std::lock_guard<std::mutex> guard(m_DescriptorLock);

	// TODO: Configurable?
	constexpr uint32_t kMaxAllocationPerPool = 2046;

	// Local copy for pool writing
	VkDescriptorSetAllocateInfo set_info = info;

	// Search last to front
	VkResult result = VK_ERROR_OUT_OF_POOL_MEMORY;
	for (auto it = m_DescriptorPools.rbegin(); it != m_DescriptorPools.rend(); it++)
	{
		set_info.descriptorPool = *it;

		// Attempt to allocate
		if ((result = m_DeviceTable->m_AllocateDescriptorSets(m_Device, &set_info, &out->m_Set)) == VK_SUCCESS)
		{
			break;
		}
	}

	// TODO: Possible return codes { OOHM, OODM, FRAGMENTED, OOPM }
	if (result != VK_SUCCESS)
	{
		// At this point no pools can accomodate for the allocation
		VkDescriptorPool pool;

		// Pool storage information
		VkDescriptorPoolSize pool_size;
		pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_size.descriptorCount = 1 + kMaxAllocationPerPool * m_Registry->GetAllocatedStorageUIDs();

		// Attempt to create descriptor pool
		VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.maxSets = kMaxAllocationPerPool;
		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes = &pool_size;
		pool_info.poolSizeCount = 1;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		if ((result = m_DeviceTable->m_CreateDescriptorPool(m_Device, &pool_info, m_Allocator, &pool)) != VK_SUCCESS)
		{
			return result;
		}

		// Attempt to allocate
		// Considered fatal at this point
		set_info.descriptorPool = pool;
		if ((result = m_DeviceTable->m_AllocateDescriptorSets(m_Device, &set_info, &out->m_Set)) != VK_SUCCESS)
		{
			return result;
		}

		// OK
		m_DescriptorPools.push_back(pool);
	}

	out->m_Pool = set_info.descriptorPool;
	return VK_SUCCESS;
}

VkResult DiagnosticAllocator::FreeDescriptorSet(const SDiagnosticDescriptorBinding & binding)
{
	std::lock_guard<std::mutex> guard(m_DescriptorLock);

	VkResult result = m_DeviceTable->m_FreeDescriptorSet(m_Device, binding.m_Pool, 1, &binding.m_Set);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

SDiagnosticFence* DiagnosticAllocator::PopFence()
{
	std::lock_guard<std::mutex> guard(m_AllocationMutex);

	VkResult result;
	if (!m_FreeFences.empty())
	{
		SDiagnosticFence* fence = m_FreeFences.back();
		m_FreeFences.pop_back();
		return fence;
	}

	// Create handle
	auto fence = new SDiagnosticFence();
	fence->m_ReferenceCount = 0;

	// Attempt to create event
	VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if ((result = m_DeviceTable->m_CreateFence(m_Device, &fence_info, m_Allocator, &fence->m_Fence)) != VK_SUCCESS)
	{
		return nullptr;
	}

	// Patch the internal dispatch table
	PatchDispatchTable(m_InstanceTable, m_Device, fence->m_Fence);

	return fence;
}

bool DiagnosticAllocator::WaitForFiltering()
{
	bool waited = false;

	std::unique_lock<std::mutex> unique(m_PendingMutex);

	// Kickoff for any dangling allocations
	m_ThreadBusyWaitFlag = true;
	m_ThreadWakeVar.notify_one();

	m_ThreadDoneVar.wait(unique, [&]
	{
		bool empty = m_PendingDiagnosticData.empty();
		waited |= !empty;
		return empty;
	});

	return waited;
}

bool DiagnosticAllocator::ApplyThrottling()
{
	bool waited = false;

	std::unique_lock<std::mutex> unique(m_PendingMutex);

	bool any_throttle = false;
	for (SPendingDiagnosticAllocation& pending : m_PendingDiagnosticData)
	{
		if (pending.m_ThrottleAge++ >= m_ThrottleTreshold)
		{
			any_throttle = true;
		}
	}

	if (!any_throttle)
		return false;

	// Kickoff for any dangling allocations
	m_ThreadBusyWaitFlag = true;
	m_ThreadWakeVar.notify_one();

	m_ThreadDoneVar.wait(unique, [&]
	{
		bool throttle = false;
		for (const SPendingDiagnosticAllocation& pending : m_PendingDiagnosticData)
		{
			if (pending.m_ThrottleAge >= m_ThrottleTreshold)
			{
				throttle = true;
			}
		}

		waited |= throttle;
		return !throttle;
	});

	return waited;
}

void DiagnosticAllocator::ApplyDefragmentation()
{
	// Free all dead allocations
	{
		std::lock_guard<std::mutex> guard(m_AllocationMutex);
		for (int64_t i = static_cast<int64_t>(m_PendingAllocations.size()) - 1; i >= 0; i--)
		{
			SDiagnosticAllocation* pending = m_PendingAllocations[i];

			// Only account for finished allocations
			if (!pending->IsDone(m_Device, m_DeviceTable) || pending->m_AgeCounter++ < m_DeadAllocationThreshold)
				continue;

			// Destroy all data associated with the dead allocation
			DestroyAllocation(pending);

			// Remove dangling allocation
			m_PendingAllocations.erase(m_PendingAllocations.begin() + i);
		}
	}

	// Check for defragmentation
	{
		std::lock_guard<std::mutex> guard(m_HeapMutex);
		DefragmentHeapType(m_DeviceHeap);
		DefragmentHeapType(m_MirrorHeap);
	}
}

bool DiagnosticAllocator::CreateMemory(uint64_t alloc_size, VkMemoryPropertyFlags required_bits, SHeapMemory* out)
{
	// Get properties
	VkPhysicalDeviceMemoryProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
	m_DeviceTable->m_GetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &properties);

	// Describe allocation
	VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.allocationSize = alloc_size;

	// Determine the wanted memory types
	uint64_t previous_size = 0;
	for (uint32_t i = 0; i < properties.memoryProperties.memoryTypeCount; i++)
	{
		VkDeviceSize size = properties.memoryProperties.memoryHeaps[properties.memoryProperties.memoryTypes[i].heapIndex].size;
		if (((properties.memoryProperties.memoryTypes[i].propertyFlags & required_bits) == required_bits) && size > alloc_size && size > previous_size)
		{
			alloc_info.memoryTypeIndex = i;
			previous_size = size;
		}
	}

	// Is the memory coherent?
	out->m_IsHostCoherent = (properties.memoryProperties.memoryTypes[alloc_info.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // && !(flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	// Attempt to allocate
	VkResult result;
	if ((result = m_DeviceTable->m_AllocateMemory(m_Device, &alloc_info, nullptr, &out->m_DeviceMemory)) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool DiagnosticAllocator::AllocateOrBind(SHeapType& type, uint64_t size, SDiagnosticHeapAllocation* out)
{
	VkResult result;

	std::lock_guard<std::mutex> guard(m_HeapMutex);

	// Get requirements
	VkMemoryRequirements requirements;
	m_DeviceTable->m_GetBufferMemoryRequirements(m_Device, out->m_HeapBuffer, &requirements);

	// Previously available allocation?
	// May be rebound
	if (!out->m_Binding.m_Heap)
	{
		out->m_HeapSpan = size;

		// Allocate from heap type
		if (!Allocate(type, requirements.alignment, requirements.size, &out->m_Binding))
			return false;
	}
	else
	{
		// According to the specification this should never happen
		// But check anyway!
		if (requirements.alignment != out->m_Binding.m_AllocationIt->m_Alignment || requirements.size != out->m_Binding.m_AllocationIt->m_Size)
		{
			if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Inconsistent vulkan memory requirements for uniform creation parameters, this is against the specification");
			}
			return false;
		}
	}

	// Bind to offset
	if ((result = m_DeviceTable->m_BindBufferMemory(m_Device, out->m_HeapBuffer, out->m_Binding.m_Heap->m_Memory.m_DeviceMemory, out->m_Binding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
	{
		return false;
	}

	// Prevent "unauthorized" usage to newly bound range
	// Also required for latent-enabled message filtering
	if (out->m_Binding.m_Heap->m_CoherentlyMappedData)
	{
		std::memset(static_cast<uint8_t*>(out->m_Binding.m_Heap->m_CoherentlyMappedData) + out->m_Binding.m_AllocationIt->m_Offset, 0xFF, size);
		if (!out->m_Binding.m_Heap->m_Memory.m_IsHostCoherent)
		{
			VkMappedMemoryRange range{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			range.memory = out->m_Binding.m_Heap->m_Memory.m_DeviceMemory;
			range.offset = out->m_Binding.m_AllocationIt->m_Offset;
			range.size = size;
			if ((result = m_DeviceTable->m_FlushMappedMemoryRanges(m_Device, 1, &range)) != VK_SUCCESS)
			{
				return false;
			}
		}
	}

	return true;
}

bool DiagnosticAllocator::Allocate(SHeapType& type, uint64_t alignment, uint64_t size, SDiagnosticHeapBinding* out)
{
	if (size > m_DeviceTable->m_CreateInfoAVA.m_ChunkedWorkingSetByteSize)
	{
		if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
		{
			char buffer[512];
			FormatBuffer(buffer, "Working group size too small, an allocation size of %i bytes forcing a dedicated allocation", static_cast<uint32_t>(size));
			m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
		}

		{
			// Prepare heap
			SHeap heap;
			heap.m_Size = size;

			// Attempt to create memory
			if (!CreateMemory(size, type.m_RequiredFlags, &heap.m_Memory))
				return false;

			// Attempt to map coherent host storage
			if (type.m_RequiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			{
				if (m_DeviceTable->m_MapMemory(m_Device, heap.m_Memory.m_DeviceMemory, 0, size, 0, reinterpret_cast<void**>(&heap.m_CoherentlyMappedData)) != VK_SUCCESS)
				{
					if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
					{
						m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Failed to map coherent host mirror stream");
					}
				}
			}

			type.m_Heaps.push_back(heap);
		}

		SHeap* heap = &type.m_Heaps.back();
		return AllocateBefore(heap, heap->m_Allocations.end(), 0, alignment, size, out);
	}
	else
	{
		for (auto heap_it = type.m_Heaps.rbegin(); heap_it != type.m_Heaps.rend(); heap_it++)
		{
			SHeap* heap = &*heap_it;

			// End-point allocation takes priority
			{
				uint64_t aligned_offset = AlignUpper((heap->m_Allocations.empty() ? 0 : (heap->m_Allocations.back().m_Offset + heap->m_Allocations.back().m_Size)), alignment);

				// Must have enough space
				if (aligned_offset + size <= heap->m_Size)
				{
					// Insert before
					return AllocateBefore(heap, heap->m_Allocations.end(), aligned_offset, alignment, size, out);
				}
			}

			// Search between allocations
			for (auto it = heap->m_Allocations.begin(), next = std::next(it); next != heap->m_Allocations.end(); it = next, ++next)
			{
				// Skip unstable allocations
				if (it->m_RebindRequest.m_Requested || next->m_RebindRequest.m_Requested)
					continue;

				// Get safe start
				uint64_t aligned_offset = AlignUpper(it->m_Offset + it->m_Size, alignment);
				if (aligned_offset > next->m_Offset)
					continue;

				// Get the working set size
				uint64_t aligned_working_set = next->m_Offset - aligned_offset;

				// Must have enough space
				if (aligned_offset + size > aligned_working_set)
					continue;

				// Insert before
				return AllocateBefore(heap, next, aligned_offset, alignment, size, out);
			}
		}
	}

	// Diagnostic
	if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
	{
		char buffer[256];
		FormatBuffer(buffer, "Allocating new [%s] diagnostics heap, high frequency allocations is the result of a low working set byte size", (type.m_RequiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? "HOST" : "DEVICE");
		m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
	}

	// Prepare heap
	SHeap heap;
	heap.m_Size = m_DeviceTable->m_CreateInfoAVA.m_ChunkedWorkingSetByteSize;
	heap.m_CoherentlyMappedData = nullptr;

	// Attempt to create memory
	if (!CreateMemory(m_DeviceTable->m_CreateInfoAVA.m_ChunkedWorkingSetByteSize, type.m_RequiredFlags, &heap.m_Memory))
		return false;

	// Attempt to map coherent host storage
	if (type.m_RequiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		if (m_DeviceTable->m_MapMemory(m_Device, heap.m_Memory.m_DeviceMemory, 0, heap.m_Size, 0, reinterpret_cast<void**>(&heap.m_CoherentlyMappedData)) != VK_SUCCESS)
		{
			if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Failed to map coherent host mirror stream");
			}
		}
	}

	type.m_Heaps.push_back(heap);
	return Allocate(type, alignment, size, out);
}

void DiagnosticAllocator::Free(SHeap* heap, SHeap::TAllocationIterator it)
{
	std::lock_guard<std::mutex> guard(m_HeapMutex);

	heap->m_Allocations.erase(it);
}

bool DiagnosticAllocator::AllocateBefore(SHeap* heap, SHeap::TAllocationIterator it, uint64_t aligned_offset, uint64_t alignment, uint64_t size, SDiagnosticHeapBinding * out)
{
#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
	// Live range check
	{
		auto range = std::make_pair(
			aligned_offset,
			aligned_offset + size
		);

		// Live range check
		heap->CheckGPURangeOverlap(range.first, range.second);

		if (heap->m_AllocationsOffsets.count(aligned_offset)) throw std::exception();
		heap->m_AllocationsOffsets.insert(aligned_offset);
	}
#endif

	// Track allocation
	SHeapAllocation allocation;
	allocation.m_Offset = aligned_offset;
	allocation.m_Alignment = alignment;
	allocation.m_Size = size;
	allocation.m_RebindRequest.m_Requested = false;

	// Append
	out->m_Heap = heap;
	out->m_AllocationIt = heap->m_Allocations.insert(it, allocation);
	out->m_MappedData = heap->m_CoherentlyMappedData ? (static_cast<uint8_t*>(heap->m_CoherentlyMappedData) + aligned_offset) : nullptr;
	return true;
}

void DiagnosticAllocator::DefragmentHeapType(SHeapType& type)
{
	for (SHeap& heap : type.m_Heaps)
	{
		if (heap.m_Allocations.empty())
			continue;

		// Current allocation candiate
		// First allocation is first candidate, offset is considered the aligned working space by default
		struct
		{
			SHeap::TAllocationIterator m_It;
			size_t					   m_AlignedWorkingSpace;
			uint64_t				   m_AlignedOffset;
		} candidate {
			heap.m_Allocations.begin(),
			heap.m_Allocations.front().m_Offset,
			0
		};

		// May be requested already
		if (heap.m_Allocations.front().m_RebindRequest.m_Requested)
		{
			candidate.m_AlignedWorkingSpace = 0;
		}

		// TODO: Search direction?
		for (auto it = heap.m_Allocations.begin(), next = std::next(it); next != heap.m_Allocations.end(); it = next, ++next)
		{
			// Skip unstable allocations
			if (it->m_RebindRequest.m_Requested || next->m_RebindRequest.m_Requested)
				continue;

			uint64_t aligned_next_begin = AlignUpper(it->m_Offset + it->m_Size, next->m_Alignment);
			if (aligned_next_begin > next->m_Offset)
				continue;

			uint64_t aligned_working_space = next->m_Offset - aligned_next_begin;

			// Better candidate?
			if (aligned_working_space > candidate.m_AlignedWorkingSpace)
			{
				candidate.m_It = next;
				candidate.m_AlignedOffset = aligned_next_begin;
				candidate.m_AlignedWorkingSpace = aligned_working_space;

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
				if (heap.m_AllocationsOffsets.count(candidate.m_AlignedOffset)) throw std::exception();
#endif
			}
		}

		// Any candidate?
		if (!candidate.m_AlignedWorkingSpace)
			continue;

		// Diagnostic
		if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
		{
			char buffer[256];
			FormatBuffer(buffer, "Defragmentation requested for empty [%s] span of %i bytes", (type.m_RequiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? "HOST" : "DEVICE", candidate.m_AlignedWorkingSpace);
			m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
		}

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
		if (!heap.m_AllocationsOffsets.count(candidate.m_It->m_Offset)) throw std::exception();
		heap.m_AllocationsOffsets.erase(candidate.m_It->m_Offset);
		
		if (heap.m_AllocationsOffsets.count(candidate.m_AlignedOffset)) throw std::exception();
		heap.m_AllocationsOffsets.insert(candidate.m_AlignedOffset);
#endif

		// Prepare request
		candidate.m_It->m_RebindRequest.m_RebindOffset = candidate.m_AlignedOffset;

		// Mark as requested
		// ! Order is important
		candidate.m_It->m_RebindRequest.m_Requested = true;
	}
}

void DiagnosticAllocator::ReleaseHeapType(SHeapType & type)
{
	for (SHeap& heap : type.m_Heaps)
	{
		if (!heap.m_Allocations.empty())
		{
			if (m_DeviceTable->m_CreateInfoAVA.m_LogCallback && (m_DeviceTable->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				m_DeviceTable->m_CreateInfoAVA.m_LogCallback(m_DeviceTable->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Diagnostics heap has dangling allocations!");
			}
		}

		m_DeviceTable->m_FreeMemory(m_Device, heap.m_Memory.m_DeviceMemory, nullptr);
	}
}

VkResult DiagnosticAllocator::CreateLayout()
{
	VkResult result;

	uint32_t storage_count = 0;
	m_Registry->EnumerateStorage(nullptr, &storage_count);

	m_LayoutStorageInfo.resize(storage_count);
	m_Registry->EnumerateStorage(m_LayoutStorageInfo.data(), &storage_count);

	VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	DeviceDispatchTable::Get(GetKey(m_Device))->m_GetPhysicalDeviceProperties2(m_PhysicalDevice, &properties);

	// Translate bindings
	std::vector<VkDescriptorSetLayoutBinding> m_Bindings(1 + storage_count);
	for (size_t i = 0; i < storage_count; i++)
	{
		VkDescriptorSetLayoutBinding& binding = m_Bindings[i];
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.descriptorCount = 1;
		binding.binding = m_LayoutStorageInfo[i].m_UID;
	}

	// Base message buffer
	{
		VkDescriptorSetLayoutBinding& binding = m_Bindings[storage_count];
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.descriptorCount = 1;
		binding.binding = 0;
	}

	// Attempt to create set layout
	VkDescriptorSetLayoutCreateInfo set_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	set_layout_info.pBindings = m_Bindings.data();
	set_layout_info.bindingCount = (m_SetLayoutBindingCount = static_cast<uint32_t>(m_Bindings.size()));
	if ((result = m_DeviceTable->m_CreateDescriptorSetLayout(m_Device, &set_layout_info, m_Allocator, &m_SetLayout)) != VK_SUCCESS)
	{
		return result;
	}

	// Attempt to create compatible pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.pSetLayouts = &m_SetLayout;
	pipeline_layout_info.setLayoutCount = 1;
	if ((result = m_DeviceTable->m_CreatePipelineLayout(m_Device, &pipeline_layout_info, m_Allocator, &m_PipelineLayout)) != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

size_t DiagnosticAllocator::RebindHeapAllocation(SHeap* heap, SHeap::TAllocationIterator it)
{
	std::lock_guard<std::mutex> guard(m_HeapMutex);

	size_t working_set = it->m_Offset - it->m_RebindRequest.m_RebindOffset;

	// Set new offset
	it->m_Offset = it->m_RebindRequest.m_RebindOffset;

	// Mark as finished
	// ! Order is important
	it->m_RebindRequest.m_Requested = false;

	// Debug
#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
	// Total range check
	{
		for (auto _it = heap->m_Allocations.begin(), next = std::next(_it); next != heap->m_Allocations.end(); _it = next, ++next)
		{
			if (_it->m_Offset + _it->m_Size > next->m_Offset)
			{
				throw std::exception();
			}
		}

		if (heap->m_Allocations.back().m_Offset + heap->m_Allocations.back().m_Size > heap->m_Size)
		{
			throw std::exception();
		}
	}
#endif

	return working_set;
}

void DiagnosticAllocator::ThreadEntry_MessageFiltering()
{
    SCommandBufferVersion commandBufferVersion;

	for (; !m_ThreadExitFlag;)
	{
		SPendingDiagnosticAllocation pending;

		// Wait for queued work
		size_t pending_count = 0;
		{
			std::unique_lock<std::mutex> unique(m_PendingMutex);
			m_ThreadWakeVar.wait(unique, [&]
			{
				pending_count = m_PendingDiagnosticData.size();
				if (pending_count)
				{
					pending = m_PendingDiagnosticData.back();
					m_PendingDiagnosticData.pop_back();
					return true;
				}

				return m_ThreadExitFlag.load() || m_ThreadBusyWaitFlag;
			});
		}

		// Busy waits
		if (pending_count == 0)
		{
			std::lock_guard<std::mutex> unique(m_PendingMutex);
			m_ThreadBusyWaitFlag = false;
			m_ThreadDoneVar.notify_all();
			continue;
		}

		// Get heap allocation
		const SDiagnosticHeapAllocation& heap_allocation = pending.m_Allocation->m_HeapAllocation;

		// Diagnostic data is always first
		auto diagnostic_data = reinterpret_cast<SDiagnosticData*>(heap_allocation.m_Binding.m_MappedData);

		// Check for corruption
#if DIAGNOSTIC_ALLOCATOR_DEBUG_CHECK
		if (diagnostic_data->m_Debug != kDebugDefault && diagnostic_data->m_Debug != kDebugMoved)
		{
			throw std::exception();
		}
#endif

        // Flush previous version
        commandBufferVersion.Flush();

		// Pass through registry
		uint32_t count = m_Registry->Handle(commandBufferVersion, diagnostic_data, nullptr);
		if (m_DeviceState->m_ActiveReport)
		{
			m_DeviceState->m_ActiveReport->m_RecievedMessages += count;
		}

		// Free up
		{
			std::lock_guard<std::mutex> unique(m_ThreadDiagnosticMirrorPoolMutex);
			m_ThreadDiagnosticMirrorPool.push_back(pending.m_Allocation);
		}
	}
}
