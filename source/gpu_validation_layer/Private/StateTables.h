#pragma once

#include "DispatchTables.h"
#include "Pipeline.h"
#include <unordered_map>
#include <memory>

class DiagnosticAllocator;
class SDiagnosticAllocation;
class DiagnosticRegistry;
class ShaderCache;
class ShaderCompiler;
class PipelineCompiler;

struct STrackedDeviceMemory
{
    bool                  m_IsMapped = false;
    std::vector<VkBuffer> m_Buffers;
    std::vector<VkImage > m_Images;
};

static constexpr uint32_t kPQIMissedFrameThreshold = 10;

struct SPendingQueueInitialization
{
    struct SSubmission
    {
        VkCommandBuffer m_CommandBuffer = nullptr;
        VkFence         m_Fence = nullptr;
    };

    uint32_t                 m_MissedFrameCounter;
    VkCommandPool            m_Pool;
    std::vector<SSubmission> m_PendingSubmissions;
    SSubmission              m_CurrentSubmission{};
};

// Represents generic statistics for debugging
struct SDeviceStatistics
{
    std::atomic<uint32_t> m_BreadcrumbDescriptorUpdates { 0 };
    std::atomic<uint32_t> m_BreadcrumbDispatchedDescriptorUpdates {0 };
};

struct DeviceStateTable
{
	/**
	 * Add a new entry
	 * @param[in] key the key of the entry
	 * @param[in] state the state
	 */
	static void Add(void* key, DeviceStateTable* state)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table[key] = state;
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
	static DeviceStateTable* Get(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return Table[key];
	}

	std::unique_ptr<DiagnosticAllocator> m_DiagnosticAllocator; ///< The shared diagnostics allocator
	std::unique_ptr<DiagnosticRegistry>  m_DiagnosticRegistry;  ///< The shared diagnostics registry
	std::unique_ptr<ShaderCache>   		 m_ShaderCache;		    ///< The shared shader cache
	std::unique_ptr<ShaderCompiler>      m_ShaderCompiler;	    ///< The shared shader compiler
	std::unique_ptr<PipelineCompiler>    m_PipelineCompiler;	///< The shared pipeline compiler

	VkPhysicalDeviceProperties2 m_PhysicalDeviceProperties; /// The device properties

	VkGPUValidationReportAVA m_ActiveReport = nullptr; ///< The active report
	std::mutex				 m_ReportLock;			   ///< Lock operations are synchronous

	uint32_t	   m_PresentAutoSerializationLastPending = 0; ///< The last recorded pending count
	uint32_t	   m_PresentAutoSerializationCounter     = 0; ///< The counter at which the last pending count did not change
	SSparseCounter m_WaitForFilterMessageCounter;			  ///< Message counter to avoid spamming

	VkQueue				  m_TransferQueue = nullptr;	  ///< The dedicated transfer queue
	VkCommandPool		  m_TransferPool = nullptr;		  ///< The dedicated transfer command pool
	std::vector<uint32_t> m_QueueFamilyIndices;           ///< The set of all possible family indices to be used
	uint32_t			  m_DedicatedTransferQueueFamily; ///< The dedicated transfer queue family index
	std::mutex			  m_TransferPoolMutex;			  ///< Assocaited lock

	VkQueue				  m_CopyEmulationQueue = nullptr;		///< The dedicated transfer queue
	VkQueue				  m_EmulatedTransferQueue = nullptr;
	VkCommandPool		  m_CopyEmulationPool = nullptr;		///< The dedicated transfer command pool
	uint32_t			  m_DedicatedCopyEmulationQueueFamily;	///< The dedicated transfer queue family index
	std::mutex			  m_CopyEmulationPoolMutex;				///< Assocaited lock

	std::unordered_map<VkCommandPool, uint32_t>   m_CommandPoolFamilyIndices;    ///< Command pool family index lookup
	std::unordered_map<VkCommandBuffer, uint32_t> m_CommandBufferFamilyIndices;  ///< Command buffer family index lookup
	std::mutex									  m_CommandFamilyIndexMutex;	 ///< Assocaited lock

    std::unordered_map<VkImage, VkDeviceMemory>			        m_ResourceImageMemory;				///< The tracked image memory sources
    std::unordered_map<VkImage, VkImageCreateInfo>				m_ResourceImageSources;				///< The tracked image creation structures
    std::unordered_map<VkImageView, VkImageViewCreateInfo>	    m_ResourceImageViewSources;			///< The tracked image view creation structures
    std::unordered_map<VkBuffer, VkDeviceMemory>			    m_ResourceBufferMemory;				///< The tracked buffer memory sources
    std::unordered_map<VkBufferView, VkBufferViewCreateInfo>    m_ResourceBufferViewSources;		///< The tracked buffer view creation structures
    std::unordered_map<VkDeviceMemory, STrackedDeviceMemory>    m_ResourceDeviceMemory;		        ///< The tracked device memory for map tracking
    std::unordered_map<VkFramebuffer, std::vector<VkImageView>> m_ResourceFramebufferSources;		///< The tracked framebuffer views
	std::unordered_map<VkRenderPass, uint32_t>					m_ResourceRenderPassDepthSlots;		///< The tracked renderpass depth attachment slots
	std::unordered_map<std::string, HShaderModule*>				m_ResourceShaderModuleLUT;			///< The module name lookup table
	std::vector<HShaderModule*>									m_ResourceShaderModuleSwapTable;	///< The tracked shader modules
	std::vector<HPipeline*>										m_ResourcePipelineSwapTable;		///< The tracked pipelines
	std::vector<HDescriptorPool*>								m_ResourceDescriptorPoolSwaptable;	///< The tracked descriptor pools
    std::unordered_map<void*, std::string>                      m_ResourceDebugNames;               ///< The tracked resource names
	std::mutex													m_ResourceLock;						/// The per device resource tracking lock

	std::mutex m_InstrumentationLock; ///< Global lock for instrumentation, to avoid mismatched pipeline <- shader modules

    std::unordered_map<VkQueue, SPendingQueueInitialization> m_FSQueues; ///< Pending per-queue operations
    std::mutex m_FSLock;                                                 ///< Global lock for first submission initialization

    SDeviceStatistics m_Statistics; ///< Per-device statistics

private:
	static std::mutex						  Mutex;
	static std::map<void*, DeviceStateTable*> Table;
};

// Represents a (potentially decayed) tracked descriptor set
struct STrackedDescriptorSet
{
	size_t		          m_CrossCompatibilityHash = 0ull;
	VkDescriptorSet		  m_NativeSet = nullptr;
	VkPipelineLayout      m_OverlappedLayout = nullptr;
	std::vector<uint32_t> m_DynamicOffsets;
};

// Represents a deferred breadcrumb descriptor set request
struct SBreadcrumbDescriptorSet
{
    HDescriptorSet* m_Queued = nullptr;
    HDescriptorSet* m_Active = nullptr;
};

struct CommandStateTable
{
	/**
	 * Add a new entry
	 * @param[in] key the key of the entry
	 * @param[in] state the dispatch state
	 */
	static void Add(void* key, CommandStateTable* state)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		Table[key] = state;
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
	static CommandStateTable* Get(void* key)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return Table[key];
	}

	SDiagnosticAllocation* m_Allocation       = nullptr;                              ///< The current diagnostic allocation
	HPipeline*			   m_ActivePipelines[kTrackedPipelineBindPoints]{};           ///< The active pipeline
    VkPipeline			   m_ActiveUnwrappedPipelines[kTrackedPipelineBindPoints]{};  ///< The active pipeline
    VkPipeline			   m_ActiveInternalPipelines[kTrackedPipelineBindPoints]{};   ///< The active pipeline
	VkPipelineBindPoint	   m_ActivePipelineBindPoint;                                 ///< The current user bind point
	STrackedDescriptorSet  m_ActiveComputeSets[kMaxBoundDescriptorSets]{};            ///< The current (potentially decayed) active compute descriptor sets
	VkRenderPassBeginInfo  m_ActiveRenderPass = {};                                   ///< The active render pass
	uint32_t			   m_ActiveFeatures   = 0;		                              ///< The active feature set of the allocation, separated from the active report

    SBreadcrumbDescriptorSet m_BreadcrumbDescriptorSets[kMaxBoundDescriptorSets]{};   ///< The stacked breadcrumb descriptor sets
    bool                     m_DirtyBreadcrumb = false;                               ///< Any pending breadcrumbs for submission?

    char m_CachedPCData[1024]; ///< All cached push constant data for injection restoration

private:
	static std::mutex						   Mutex;
	static std::map<void*, CommandStateTable*> Table;
};

/**
 * Get the exported object information from a given descriptor
 * @param state the shared device state
 * @param descriptor the (versioned) descriptor
 * @return the object info, may not be populated if the underlying resource was not mapped or found
 */
VkGPUValidationObjectInfoAVA GetDescriptorObjectInfo(DeviceStateTable* state, const STrackedWrite& descriptor);
