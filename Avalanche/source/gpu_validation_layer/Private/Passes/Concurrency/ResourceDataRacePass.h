#pragma once

#include <DiagnosticRegistry.h>
#include <DiagnosticAllocator.h>
#include <map>
#include <unordered_map>

namespace Passes
{
	namespace Concurrency
	{
		enum class ResourceDataRaceValidationErrorType
		{
			eUnsafeWrite,
			eUnsafeRead
		};

		struct ResourceDataRaceDescriptorStorage
		{
			uint32_t			   m_DOICount;
			VkBuffer			   m_Buffer;
			SDiagnosticHeapBinding m_Binding;
		};

		class ResourceDataRacePass : public IDiagnosticPass
		{
		public:
			ResourceDataRacePass(DeviceDispatchTable* table, DeviceStateTable* state);

			/// Overrides
			void Initialize(VkCommandBuffer cmd_buffer) override;
			void Release() override;
			void EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count) override;
			void EnumerateDescriptors(SDiagnosticDescriptorInfo* storage, uint32_t* count) override;
			void EnumeratePushConstants(SDiagnosticPushConstantInfo* constants, uint32_t* count) override;
			size_t UpdatePushConstants(VkCommandBuffer buffer, SPushConstantDescriptor* constants, uint8_t* data);
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
			void* GetImageViewKey(const VkImageView& view);

            /**
             * Insert a batched message
             * @param[in] version the sequential command buffer version
             * @param[in] key prefetched key
             * @param[in] message the message contents
             * @param[in] count the total batched count
             */
			void InsertBatched(SCommandBufferVersion &version, uint64_t key, const SDiagnosticMessageData& message, uint32_t count);

			/**
			 * Create a new descriptor storage
			 * @param[in] doi_count the descriptor of interest count
			 * @param[out] out the created storage
			 */
			VkResult CreateStorage(uint32_t doi_count, ResourceDataRaceDescriptorStorage **out);

			DeviceDispatchTable* m_Table;					///< The device dispatch table
			DeviceStateTable*	 m_StateTable;				///< The device state table
			uint16_t			 m_ErrorUID;				///< The registry error uid
			uint16_t			 m_GlobalLockDescriptorUID; ///< The per descriptor set global lock buffer descriptor uid
			uint16_t			 m_MetadataDescriptorUID;   ///< The per descriptor set metadata buffer descriptor uid
			uint16_t			 m_DescriptorStorageUID;	///< The per descriptor set storage uid
			uint16_t			 m_DrawIDPushConstantUID;	///< The per cmd push constant uid

			ResourceDataRaceDescriptorStorage*			    m_DummyStorage;	///< Dummy storage for sets with no DOIs
			std::vector<ResourceDataRaceDescriptorStorage*> m_StoragePool;	///< Pool of available storages
			std::mutex										m_StorageLock;	///< General pool lock

			std::atomic<uint32_t> m_SharedIDCounter{ 0 }; ///< Push uid counter

			VkBuffer			   m_GlobalLockBuffer;	   ///< The global lock buffer, contains all registered locks
			VkBufferView		   m_GlobalLockBufferView; ///< The respective view
			SDiagnosticHeapBinding m_GlobalLockBinding;	   ///< The respective binding

			std::unordered_map<void*, uint32_t>	   m_LockOffsets;    ///< All key lock uids
			std::mutex							   m_LockOffsetLock; ///< Associated lock
			std::unordered_map<VkImageView, void*> m_ImageViewKeys;  ///< Keys of image views, takes subresources into account

			std::vector<VkGPUValidationMessageAVA> m_Messages;					  ///< All batched messages
			std::map<uint64_t, uint64_t>		   m_MessageLUT;				  ///< Batched message lookup table
			uint32_t							   m_AccumulatedStepMessages = 0; ///< Step accumulation
		};
	}
}