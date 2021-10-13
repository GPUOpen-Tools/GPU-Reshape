#pragma once

#include <DiagnosticRegistry.h>
#include <StateTables.h>
#include <Allocation.h>
#include <map>

namespace Passes
{
	namespace Basic
	{
		enum class RuntimeArrayBoundsValidationResourceType
		{
			eImage,
			eBuffer
		};

		struct RuntimeArrayBoundsDescriptorStorage
		{
			uint32_t			   m_DOICount;
			VkBuffer			   m_Buffer;
			VkBufferView		   m_BufferView;
			SDiagnosticHeapBinding m_Binding;
		};

		class RuntimeArrayBoundsPass : public IDiagnosticPass
		{
		public:
			RuntimeArrayBoundsPass(DeviceDispatchTable* table, DeviceStateTable* state);

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

		private:
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
			VkResult CreateStorage(uint32_t doi_count, RuntimeArrayBoundsDescriptorStorage **out);

            DeviceDispatchTable* m_Table;    ///< The shared device table
            DeviceStateTable*    m_State;    ///< The shared device state
			uint16_t			 m_ErrorUID;			 ///< The registry error uid
			uint16_t			 m_DescriptorUID;		 ///< The registry descriptor index uid
			uint16_t			 m_DescriptorStorageUID; ///< The per descriptor set storage uid

			RuntimeArrayBoundsDescriptorStorage*			  m_DummyStorage; ///< Dummy storage for sets with no DOIs
			std::vector<RuntimeArrayBoundsDescriptorStorage*> m_StoragePool;  ///< Pool of available storages
			std::mutex										  m_StorageLock;  ///< General pool lock

			std::vector<VkGPUValidationMessageAVA> m_Messages;					  ///< All batched messages
			std::map<uint64_t, uint64_t>		   m_MessageLUT;				  ///< Batched message lookup table
			uint32_t							   m_AccumulatedStepMessages = 0; ///< Step accumulation
		};
	}
}