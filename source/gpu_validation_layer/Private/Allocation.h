#pragma once

#include <atomic>
#include <list>
#include <condition_variable>
#include <thread>
#include <set>
#include <algorithm>

// Perform safety checks upon defragmentation
#ifndef DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
#	define DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK 0
#endif

// Debugging the allocation ranges is a pain in the ass with optimizations on
#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
#	pragma optimize ("", off)
#endif

// Represents a diagnostics storage
struct SDiagnosticStorage
{
    VkBuffer			   m_Buffer;
    VkDescriptorBufferInfo m_Descriptor;
    uint64_t			   m_HeapOffset;
};

// Represents a grouped diagnostics fence
struct SDiagnosticFence
{
    VkFence				  m_Fence;
    std::atomic<uint32_t> m_ReferenceCount;
};

// Represents device memory allocation
struct SHeapMemory
{
    VkDeviceMemory m_DeviceMemory;
    bool		   m_IsHostCoherent;
};

// Represents a defragmentation request
struct SDiagnosticHeapRebindRequest
{
    bool			 m_Requested;    ///< Set after request has been pushed
    size_t			 m_RebindOffset; ///< The new requested offset
};

// Represents a single allocation within a heap
struct SHeapAllocation
{
    SDiagnosticHeapRebindRequest m_RebindRequest;
    size_t						 m_Alignment;
    size_t						 m_Offset;
    size_t						 m_Size;
};

// Represents a single heap allocation
struct SHeap
{
    void*					   m_CoherentlyMappedData;
    SHeapMemory				   m_Memory;
    size_t					   m_Size;
    std::list<SHeapAllocation> m_Allocations;

    // Type helper
    using TAllocationIterator = decltype(m_Allocations)::iterator;

#if DIAGNOSTIC_ALLOCATOR_DEFRAGMENTATION_CHECK
    struct SLiveRange
	{
		std::pair<uint64_t, uint64_t> m_MemoryRange;
		TAllocationIterator m_Alloc;
	};

	std::vector<SLiveRange> m_LiveGPURanges;
	std::set<uint64_t> m_AllocationsOffsets;

	void CheckGPURangeOverlap(uint64_t begin, uint64_t end)
	{
		for (auto live : m_LiveGPURanges)
		{
			if (begin <= live.m_MemoryRange.second && live.m_MemoryRange.first <= end)
			{
				throw std::exception();
			}

			bool _break = false;
			(void)_break;
		}
	}
#endif
};

// Represents a dedicated heap type
struct SHeapType
{
    SHeapType(VkMemoryPropertyFlags required_flags) : m_RequiredFlags(required_flags)
    {
        /* */
    }

    VkMemoryPropertyFlags m_RequiredFlags;
    std::list<SHeap> m_Heaps;
};

// Represents the binding within a heap
struct SDiagnosticHeapBinding
{
    SHeap*						  m_Heap = nullptr;
    SHeap::TAllocationIterator    m_AllocationIt;
    void*						  m_MappedData;
};

// Represents the binding of a descriptor set
struct SDiagnosticDescriptorBinding
{
    VkDescriptorPool m_Pool;
    VkDescriptorSet m_Set;
};

// Represents an allocation binding within a heap
struct SDiagnosticHeapAllocation
{
    SDiagnosticHeapBinding m_Binding;
    uint64_t			   m_HeapSpan;
    VkBuffer			   m_HeapBuffer;
    VkBufferCreateInfo     m_CreateInfo;
};

// Represents the host mirrored allocation
struct SMirrorAllocation
{
    uint32_t				  m_MessageLimit;
    SDiagnosticHeapAllocation m_HeapAllocation;
};

// Represents a latent transfer
struct SAllocationTransfer
{
    size_t   m_ByteSpan;
    uint32_t m_MessageCount;
};

// Represents a diagnostics allocation
// All diagnostics data is hosted within this
struct SDiagnosticAllocation
{
    SDiagnosticHeapAllocation		m_DeviceAllocation;		   ///< Constant per allocation
    SMirrorAllocation*				m_MirrorAllocation;		   ///< Variable per allocation
    uint32_t						m_MessageLimit;			   ///< The constant message limit of this allocation
    uint32_t						m_ThrottleIndex;		   ///< The throttling index, currently unused
    uint32_t						m_AgeCounter;			   ///< The number of frames where this allocation was not used
    std::vector<SDiagnosticStorage> m_Storages;				   ///< Unused
    VkDescriptorBufferInfo			m_BufferDescriptor;		   ///< The descriptor for the device local data
    VkDescriptorSet					m_DescriptorSet;		   ///< The descriptor set to be used with an allocation
    VkDescriptorPool				m_DescriptorPool;		   ///< The descriptor pool from which the set was allocated
    VkCommandBuffer					m_TransferCmdBuffer;	   ///< The dedicated transfer command buffer, may be ignored
    VkSemaphore						m_TransferSignalSemaphore; ///< The dedicated transfer signal coupled with the command buffer
    bool							m_IsTransferSyncPoint;	   ///< Is this allocation a viable scheduling synchronization point?
    bool							m_PendingTransferSync;     ///< Is this allocation still "waiting" for a sync point?
    uint32_t						m_SourceFamilyIndex;	   ///< The originating family index for queue ownership transitions

    uint32_t m_LastMessageCount;	   ///< The previously recorded message count
    uint64_t m_ActiveTag;			   ///< The owning tag of this allocation for limit tracking
    uint32_t m_ActiveTagLatentCount;   ///< The latent message count of the owning tag

    uint32_t m_DebugData = 42;		   ///< Useful for debugging corrupted data

    /**
     * Busy lock this allocation
     */
    void Lock()
    {
        while (!m_Lock.test_and_set(std::memory_order_acquire));
    }

    /**
     * Busy unlock this allocation
     */
    void Unlock()
    {
        m_Lock.clear(std::memory_order_release);
    }

    /**
     * Flag that this allocation does not require a fence check
     */
    void SkipFence()
    {
        Lock();
        m_SkipFence = true;
        Unlock();
    }

    /**
     * Set the fence, does not increase the reference count
     * @param[in] fence the fence to be associated with this allocation
     */
    void SetFence(SDiagnosticFence* fence)
    {
        Lock();
        m_Fence = fence;
        m_CachedDone = false;
        Unlock();
    }

    /**
     * Is this allocation done?
     * @param[in] device the vulkan device
     * @param[in] table the dispatch table associated with <device>
     */
    bool IsDone(VkDevice device, DeviceDispatchTable* table)
    {
        if (m_CachedDone)
            return true;

        Lock();
        bool done = m_SkipFence || (m_Fence && table->m_GetFenceStatus(device, m_Fence->m_Fence) == VK_SUCCESS);
        m_CachedDone = done;
        Unlock();

        return done;
    }

    /**
     * Get the internal fence, not thread safe
     */
    SDiagnosticFence* GetUnsafeFence() const
    {
        return m_Fence;
    }

    /**
     * Get the latent transfer information
     * @param[in] latent allow for latent ranges
     */
    SAllocationTransfer GetTransfer(bool latent) const
    {
        SAllocationTransfer transfer;
        transfer.m_ByteSpan = m_DeviceAllocation.m_HeapSpan;
        transfer.m_MessageCount = UINT32_MAX;

        // Allow for latent transfers?
        if (latent)
        {
            transfer.m_ByteSpan = sizeof(SDiagnosticData) - sizeof(SDiagnosticMessageData);
            transfer.m_MessageCount = GetLatentMessageCount();

            if (transfer.m_MessageCount)
            {
                transfer.m_ByteSpan += (sizeof(SDiagnosticMessageData) * transfer.m_MessageCount);
            }
        }

        return transfer;
    }

    /**
     * Get the latent message count based
     */
    uint32_t GetLatentMessageCount() const
    {
        return m_ActiveTag ? m_ActiveTagLatentCount : m_LastMessageCount;
    }

    /**
     * Reset all state tracking within this allocation
     * @param[in] tag the tag to be associated with this allocation
     */
    void Reset(uint64_t tag, uint32_t tag_latent_count)
    {
        m_Fence = nullptr;
        m_SkipFence = false;
        m_CachedDone = false;
        m_ThrottleIndex = 0;
        m_AgeCounter = 0;
        m_PendingTransferSync = true;
        m_ActiveTag = tag;
        m_ActiveTagLatentCount = tag_latent_count;
    }

private:
    std::atomic_flag  m_Lock;       ///< Busy lock
    SDiagnosticFence* m_Fence;      ///< Reference counted fence associated with this allocation
    bool			  m_SkipFence;  ///< Ignore fence checking?
    bool			  m_CachedDone; ///< Avoid needless fence querying
};