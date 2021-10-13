#pragma once

#include "DiagnosticData.h"
#include "DiagnosticRegistry.h"
#include "DispatchTables.h"
#include "Allocation.h"

struct DeviceStateTable;

class DiagnosticAllocator
{
public:
	DiagnosticAllocator();

	/**
	 * Initialize this allocator
	 * @param[in] instance the vulkan instance
	 * @param[in] physicalDevice the vulkan physical originating from <instance>
	 * @param[in] device the vulkan device originating from <physicalDevice>
	 * @param[in] allocator the vulkan allocator callbacks
	 * @param[in] registry the diagnostic registry
	 */
	VkResult Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, const VkAllocationCallbacks* allocator, DiagnosticRegistry* registry);

	/**
	 * Release this allocator
	 */
	void Release();

	/**
	 * Register this allocator for use within the spirv optimizer
	 * @param[in] state the shared state
	 * @param[in] optimizer the optimizer to be registered with
	 */
	void Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer);

	/**
	 * Pop a new diagnostics allocation
	 * @param[in] cmd_buffer the vulkan command buffer used for allocation preparation
	 * @param[in] tag the tag to be tracked with, used for dynamic allocation growth
	 */
	SDiagnosticAllocation* PopAllocation(VkCommandBuffer cmd_buffer, uint64_t tag);

	/**
	 * Serially transfer the allocation data
	 * ! Must be done on the same command list as validation generation
	 * @param[in] cmd_buffer the vulkan command buffer used for allocation transferring
	 * @param[in] allocation the diagnostics allocation
	 */
	void TransferInplaceAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation* allocation);

	/**
	 * Begin the asynchronous allocation transfer
	 * ? Sets up the nessesary barriers
	 * @param[in] cmd_buffer the vulkan command buffer used for allocation transferring
	 * @param[in] allocation the diagnostics allocation
	 */
	void BeginTransferAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation* allocation);

	/**
	 * Begin the asynchronous allocation transfer
	 * ! Must execute serially to BeginTransferAllocation
	 * ? Recieved the begin event and transfers the allocation
	 * @param[in] cmd_buffer the vulkan command buffer used for allocation transferring
	 * @param[in] allocation the diagnostics allocation
	 */
	void EndTransferAllocation(VkCommandBuffer cmd_buffer, SDiagnosticAllocation* allocation);

	/**
	 * Push an allocation for deferred filtering
	 * @param[in] allocation the diagnostics allocation
	 */
	void PushAllocation(SDiagnosticAllocation* allocation);

	/**
	 * Destroy an allocation
	 * @param[in] allocation the diagnostics allocation
	 */
	void DestroyAllocation(SDiagnosticAllocation* allocation);

	/**
	 * Pop a new grouped diagnostics fence
	 */
	SDiagnosticFence* PopFence();

	/**
	 * Push all pending allocations for filtering
	 */
	uint32_t WaitForPendingAllocations();

	/**
	 * Wait for all asynchronous filtering to complete
	 */
	bool WaitForFiltering();

	/**
	 * Apply <frame> throttling to asynchronous filtering
	 * ? If the GPU is emmitting more messages than the CPU can filter at a time it means that the mirror allocations will lag behind
	 *   Worst case scenario is that we run out of memory, therefore the throttling is applied.
	 */
	bool ApplyThrottling();

	/**
	 * Apply iterative defragmentation to all allocations
	 */
	void ApplyDefragmentation();

	/**
	 * Get the shared descriptor set layout
	 */
	VkDescriptorSetLayout GetSharedSetLayout() const
	{
		return m_SetLayout;
	}

	/**
	 * Get the bind count of the shared descriptor set layout 
	 */
	uint32_t GetSharedSetLayoutBindingCount() const
	{
		return m_SetLayoutBindingCount;
	}

	/**
	 * Get the shared pipeline layout
	 */
	VkPipelineLayout GetSharedPipelineLayout() const
	{ 
		return m_PipelineLayout;
	}

	/**
	 * Set the throttle threshold
	 * @param[in] threshold the threshold to be applied, 0 implies immediate
	 */
	void SetThrottleThreshold(uint32_t threshold)
	{
		m_ThrottleTreshold = threshold;
	}

	/**
	 * Get the throttle threshold
	 */
	uint32_t GetThrotteThreshold() const
	{
		return m_ThrottleTreshold;
	}

public:
	/**
    * Create a descriptor binding
    * @param[in] alignment the requested allocation alignment
    * @param[in] size the requested allocation size
    * @param[out] out the allocation
    */
	VkResult AllocateDescriptorBinding(uint64_t alignment, uint64_t size, SDiagnosticHeapBinding* out);

	/**
    * Free a descriptor binding
    * @param[in] bind a valid descriptor binding
    */
	void FreeDescriptorBinding(const SDiagnosticHeapBinding& binding);

	/**
    * Create a device binding
    * @param[in] alignment the requested allocation alignment
    * @param[in] size the requested allocation size
    * @param[out] out the allocation
    */
	VkResult AllocateDeviceBinding(uint64_t alignment, uint64_t size, SDiagnosticHeapBinding* out);

	/**
    * Free a device binding
    * @param[in] bind a valid descriptor binding
    */
	void FreeDeviceBinding(const SDiagnosticHeapBinding& binding);

public:
    /**
    * Allocate a new descriptor set
    * @param[in] info the allocation info, pool is ignored
    * @param[out] out the binding output
    */
	VkResult AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& info, SDiagnosticDescriptorBinding* out);

    /**
    * Free a descriptor binding
    * @param[in] binding the binding to be freed
    */
	VkResult FreeDescriptorSet(const SDiagnosticDescriptorBinding& binding);

private:
	/**
	 * No copies
	 */
	DiagnosticAllocator(const DiagnosticAllocator&) = delete;
	DiagnosticAllocator& operator=(const DiagnosticAllocator&) = delete;

private:
	/**
	 * Set the throttle threshold
	 * @param[in] threshold the threshold to be applied, 0 implies immediate
	 */
	void StartMessageFiltering(SDiagnosticAllocation* allocation);

	/**
	 * Set the throttle threshold
	 * @param[in] cmd_buffer the command buffer in which to insert the commands
	 * @param[in] allocation the diagnostics allocation
	 */
	void UpdateHeader(VkCommandBuffer cmd_buffer, SDiagnosticAllocation* allocation);

	/**
	 * Set the throttle threshold
	 * @param[in] mesage_limit the required message count
	 * @param[out] out the popped mirror allocation
	 */
	VkResult PopMirrorAllocation(uint32_t message_limit, SMirrorAllocation** out);

	/**
	 * Rebind the device memory within an allocation
	 * @param[in] allocation the diagnostics allocation
	 */
	VkResult RebindAllocationDeviceMemory(SDiagnosticAllocation* allocation);

	/**
	 * Create all descriptors within an allocation
	 * @param[in] allocation the diagnostics allocation
	 */
	VkResult CreateAllocationDescriptors(SDiagnosticAllocation* allocation);

private:
	/**
	 * Set the throttle threshold
	 * @param[in] threshold the threshold to be applied, 0 implies immediate
	 */
	VkResult CreateLayout();

private:
	/**
	 * Create a new device memory allocation
	 * @param[in] size the requested block size
	 * @param[in] required_bits the required memory property type bits
	 * @param[out] out the device memory
	 */
	bool CreateMemory(uint64_t size, VkMemoryPropertyFlags required_bits, SHeapMemory* out);

	/**
	 * Dynamically allocate from a heap type
	 * @param[in] type the heap type to allocate from
	 * @param[in] alignment the requested allocation alignment
	 * @param[in] size the requested allocation size
	 * @param[out] out the allocation
	 */
	bool Allocate(SHeapType& type, uint64_t alignment, uint64_t size, SDiagnosticHeapBinding* out);

	/**
	 * Dynamically allocate from a heap type and bind the allocation
	 * @param[in] type the heap type to allocate from
	 * @param[in] size the requested allocation size
	 * @param[out] out the allocation
	 */
	bool AllocateOrBind(SHeapType& type, uint64_t size, SDiagnosticHeapAllocation* out);

	/**
	 * Free an allocation form a heap
	 * @param[in] heap the heap to free from
	 * @param[in] it the allocation iterator within <heap>
	 */
	void Free(SHeap* heap, SHeap::TAllocationIterator it);

	/**
	 * Dynamically allocate from a heap type
	 * @param[in] heap the heap to allocate from
	 * @param[in] it the allocation iterator within <heap>
	 * @param[in] aligned_offset the final aligned offset
	 * @param[in] alignment the requested allocation alignment
	 * @param[in] size the requested allocation size
	 * @param[out] out the allocation
	 */
	bool AllocateBefore(SHeap* heap, SHeap::TAllocationIterator it, uint64_t aligned_offset, uint64_t alignment, uint64_t size, SDiagnosticHeapBinding * out);

	/**
	 * Apply iterative defragmentation to all allocations
	 * @param[in] type the heap type to defragment
	 */
	void DefragmentHeapType(SHeapType& type);

	/**
	 * Release a heap type
	 * @param[in] type the heap type to release
	 */
	void ReleaseHeapType(SHeapType& type);

	/**
	 * Mark an allocation rebinding request as finished
	 * @param[in] heap the heap
	 * @param[in] it the allocation iterator within <heap>
	 */
	size_t RebindHeapAllocation(SHeap* heap, SHeap::TAllocationIterator it);

private:
	/**
	 * The message filtering thread callback
	 */
	void ThreadEntry_MessageFiltering();

	// Represents a pending diagnostics allocation ready for filtering
	struct SPendingDiagnosticAllocation
	{
		SMirrorAllocation* m_Allocation;
		uint32_t		   m_ThrottleAge;
	};

	std::atomic_bool		m_ThreadExitFlag{ false };	   ///< Toggled when a thread exit is requested
	std::atomic_bool		m_ThreadBusyWaitFlag{ false }; ///< Toggled when the async filtering is expected to complete all jobs
	std::condition_variable m_ThreadWakeVar;			   ///< Woken when a new job is available
	std::condition_variable m_ThreadDoneVar;			   ///< Woken when all jobs have been completed
	std::thread				m_Thread;					   ///< The filtering thread handle

	std::vector<SMirrorAllocation*> m_ThreadDiagnosticMirrorPool;		///< The pool of available mirror allocations
	std::mutex						m_ThreadDiagnosticMirrorPoolMutex;  ///< The associated pool lock

	std::vector<SPendingDiagnosticAllocation> m_PendingDiagnosticData;  ///< The pending allocations for filtering
	std::mutex								  m_PendingMutex;			///< The associated lock

private:
	VkDevice					 m_Device;		   ///< The vulkan device
	VkPhysicalDevice			 m_PhysicalDevice; ///< The vulkan physical device
	const VkAllocationCallbacks* m_Allocator;	   ///< The vulkan allocator
	DiagnosticRegistry*			 m_Registry;	   ///< The diagnostics registry
	DeviceDispatchTable*		 m_DeviceTable;	   ///< The device dispatch table
	DeviceStateTable*			 m_DeviceState;	   ///< The device state table
	InstanceDispatchTable		 m_InstanceTable;  ///< The instance dispatch table

	std::mutex							m_AllocationMutex;		   ///< Generic lock for common data (sue me)
	std::vector<SDiagnosticAllocation*> m_PendingAllocations;      ///< All pending (in flight) allocations
	std::vector<void*>					m_ImmediateStorageLookup;  ///< The immediate storage lookup used for filtering
																   
	std::vector<SDiagnosticFence*>		m_FreeFences;			   ///< Free fences for diagnostic allocation grouping
																   
	VkDescriptorSetLayout				m_SetLayout;			   ///< The shared descriptor set layout
	uint32_t							m_SetLayoutBindingCount;   ///< The shared descriptor set layout binding count
	VkPipelineLayout					m_PipelineLayout;		   ///< The share pipeline layout
	std::vector<VkDescriptorPool>       m_DescriptorPools;		   ///< The shared set of descriptor pools
	std::mutex							m_DescriptorLock;		   ///< The descriptor pool lock
	std::vector<SDiagnosticStorageInfo> m_LayoutStorageInfo;	   ///< The layout storage information
																   
	std::map<uint64_t, uint32_t>		m_TagMessageCounters;      ///< The message counter table
	SSparseCounter						m_MessageCounter;          ///< Sparse counter to avoid spamming
	uint32_t							m_AverageMessageCount = 0; ///< The average message count, weighted
	
	// Represents the cyclic buffer which tracks the latent message counts
	struct STagCounterBuffer
	{
		static constexpr auto kCount = 5;

		uint32_t m_Index;
		uint32_t m_Buffer[kCount]{};
	};

	std::map<uint64_t, STagCounterBuffer> m_LatentTagMessageCounter; ///< The latent counter buffer lookup

private: /* Configurable states */
	uint32_t m_ThrottleTreshold					 = 3;	  ///< The threshold at which throttling is applied
	float    m_GrowthFactor					     = 1.5f;  ///< The growth factor applied to tagged allocations 
	float    m_AllocationViabilityLimitThreshold = 2.0f;  ///< The threshold at which the allocation message limit factor is viable
	float    m_TransferSyncPointThreshold		 = 0.5f;  ///< The threshold at which the allocation requests an async transfer submission
	float    m_AverageMessageWeight			     = 0.95f; ///< The weighting applied to message counting
	uint32_t m_DeadAllocationThreshold           = 32;    ///< The threshold at which an allocation is considered dead

private: /* Preconfigured heap types */
	std::mutex m_HeapMutex;

	SHeapType m_DeviceHeap = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	SHeapType m_MirrorHeap = { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT };
	SHeapType m_DescriptorHeap = { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
};