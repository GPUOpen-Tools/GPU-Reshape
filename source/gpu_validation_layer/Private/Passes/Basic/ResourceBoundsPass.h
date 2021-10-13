#pragma once

#include <DiagnosticRegistry.h>
#include <StateTables.h>
#include <map>

namespace Passes
{
	namespace Basic
	{
		enum class ResourceBoundsValidationResourceType
		{
			eImage,
			eBuffer
		};

		class ResourceBoundsPass : public IDiagnosticPass
		{
		public:
			ResourceBoundsPass(DeviceDispatchTable* table, DeviceStateTable* state);

			/// Overrides
			void Initialize(VkCommandBuffer cmd_buffer) override;
			void Release() override;
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

            DeviceDispatchTable* m_Table;    ///< The shared device table
            DeviceStateTable*    m_State;    ///< The shared device state
			uint16_t			m_ErrorUID; ///< The registry error UID

			std::vector<VkGPUValidationMessageAVA> m_Messages;						  ///< All batched messages
			std::map<uint64_t, uint64_t>		   m_MessageLUT;					  ///< Batched message lookup table
			uint32_t							   m_AccumulatedStepMessages[2]{ 0 }; ///< The step accumulation
		};
	}
}