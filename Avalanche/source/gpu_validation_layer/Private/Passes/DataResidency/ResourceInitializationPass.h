#pragma once

#include <DiagnosticRegistry.h>
#include <DiagnosticAllocator.h>
#include <ComputeKernel.h>
#include <map>
#include <unordered_map>

namespace Passes
{
	namespace DataResidency
	{
		struct ResourceInitializationDescriptorStorage
		{
			uint32_t			   m_DOICount;
			VkBuffer			   m_RIDBuffer;
			SDiagnosticHeapBinding m_RIDBinding;
			VkBuffer			   m_RSMaskBuffer;
			SDiagnosticHeapBinding m_RSMaskBinding;
		};

		class ResourceInitializationPass : public IDiagnosticPass
		{
		public:
			ResourceInitializationPass(DeviceDispatchTable* table, DeviceStateTable* state);

			/// Overrides
			void Initialize(VkCommandBuffer cmd_buffer) override;
			void Release() override;
			void EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count) override;
			void EnumerateDescriptors(SDiagnosticDescriptorInfo* storage, uint32_t* count) override;
			void CreateDescriptors(HDescriptorSet* set) override;
			void DestroyDescriptors(HDescriptorSet* set) override;
			void UpdateDescriptors(HDescriptorSet* set, bool update, bool push, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob) override;
			uint32_t Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage) override;
			void Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer) override;
			void Step(VkGPUValidationReportAVA report) override;
			void Report(VkGPUValidationReportAVA report) override;
			void Flush() override;

		public:
			/**
			 * Command hook, begin a render pass
			 * @param[in] cmd_buffer the command buffer
			 * @param[in] info the begin info
			 */
			void BeginRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo& info);

			/**
			 * Command hook, end a render pass
			 * @param[in] cmd_buffer the command buffer
			 * @param[in] info the previous begin info
			 */
			void EndRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo& info);

			/**
			 * Command hook, buffer initialization
			 * @param[in] cmd_buffer the command buffer
			 * @param[in] info the begin info
			 */
			void InitializeBuffer(VkCommandBuffer cmd_buffer, VkBuffer buffer);

			/**
			 * Command hook, image initialization
			 * @param[in] cmd_buffer the command buffer
			 * @param[in] info the begin info
			 */
			void InitializeImage(VkCommandBuffer cmd_buffer, VkImage image, const VkImageSubresourceRange& range);

			/**
			 * Command hook, image view initialization
			 * @param[in] cmd_buffer the command buffer
			 * @param[in] info the begin info
			 */
			void InitializeImageView(VkCommandBuffer cmd_buffer, VkImageView view);

		public:
            /**
             * Free an image
             * @param[in] cmd_buffer the command buffer
             * @param[in] info the begin info
             */
            void FreeImage(VkCommandBuffer cmd_buffer, VkImage image);

            /**
             * Free a buffer
             * @param[in] cmd_buffer the command buffer
             * @param[in] info the begin info
             */
            void FreeBuffer(VkCommandBuffer cmd_buffer, VkBuffer buffer);

		private:
			/**
			 * Get the lock uid of a key
			 * ? Always > 0
			 * @param[in] key a valid key
			 */
			uint32_t GetLockUID(void* key);

			/**
			 * Get the key of an image view
			 * @param[in] view the image view
			 */
			void* GetImageViewKey(VkImageView view);

			/**
			 * Get the subresource mask of an image view
			 * @param[in] view the image view
			 */
			uint32_t GetImageViewSRMask(VkImageView view);

			/**
			 * Get the subresource mask of an image subresource range
			 * @param[in] view the image view
			 */
			uint32_t GetImageSRMask(VkImage image, VkImageSubresourceRange range);

            /**
             * Insert a batched message
             * @param[in] version the sequential command buffer version
             * @param[in] key prefetched key
             * @param[in] message the message contents
             * @param[in] count the total batched count
             */
			void InsertBatched(SCommandBufferVersion &version, SStringCache *message_cache, uint64_t key, const SDiagnosticMessageData& message, uint32_t count);

			/**
			 * Create a new descriptor storage
			 * @param[in] doi_count the descriptor of interest count
			 * @param[out] out the created storage
			 */
			VkResult CreateStorage(uint32_t doi_count, ResourceInitializationDescriptorStorage **out);

			DeviceDispatchTable* m_Table;					 ///< The device dispatch table
			DeviceStateTable*	 m_StateTable;				 ///< The device state table
			uint16_t			 m_ErrorUID;				 ///< The registry error uid
			uint16_t			 m_GlobalStateDescriptorUID; ///< The per descriptor set global state buffer descriptor uid
			uint16_t			 m_MetadataRIDDescriptorUID;    ///< The per descriptor set metadata buffer descriptor uid
			uint16_t			 m_MetadataSRMaskDescriptorUID;    ///< The per descriptor set metadata buffer descriptor uid
			uint16_t			 m_DescriptorStorageUID;	 ///< The per descriptor set storage uid

            ComputeKernel m_KernelSRMaskWrite;
            ComputeKernel m_KernelSRMaskFree;

			ResourceInitializationDescriptorStorage*			    m_DummyStorage;	///< Dummy storage for sets with no DOIs
			std::vector<ResourceInitializationDescriptorStorage*>	m_StoragePool;	///< Pool of available storages
			std::mutex												m_StorageLock;	///< General pool state

			VkBuffer			   m_GlobalStateBuffer;	   ///< The global state buffer, contains all registered states
			VkBufferView		   m_GlobalStateBufferView; ///< The respective view
			SDiagnosticHeapBinding m_GlobalStateBinding;	   ///< The respective binding

			std::unordered_map<void*, uint32_t>	   m_StateOffsets;    ///< All key state uids
			std::mutex							   m_StateOffsetLock; ///< Associated lock
			std::unordered_map<VkImageView, void*> m_ImageViewKeys;  ///< Keys of image views,
			std::unordered_map<VkImageView, uint32_t> m_ImageViewSRMasks;  ///< Subresource masks

			std::vector<uint32_t> m_GlobalStateMirror; ///< Host mirrored global state

			std::vector<VkGPUValidationMessageAVA> m_Messages;					  ///< All batched messages
			std::map<uint64_t, uint64_t>		   m_MessageLUT;				  ///< Batched message lookup table
			uint32_t							   m_AccumulatedStepMessages = 0; ///< Step accumulation
		};
	}
}