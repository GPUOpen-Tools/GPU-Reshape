#pragma once

#include <DiagnosticRegistry.h>
#include <Private/BreadcrumbAllocator.h>
#include <ComputeKernel.h>
#include <map>

struct SDiagnosticAllocation;

namespace Passes
{
    enum class EStateVersionBreadcrumbType
    {
        eDescriptorSet,
    };

    struct SStateVersionBreadcrumb
    {
        EStateVersionBreadcrumbType m_Type;

        union
        {
            struct
            {
                uint32_t m_SetIndex;
                uint32_t m_StorageIndex;
            } m_DescriptorSet;
        };
    };

    struct DescriptorSetStateUpdate
    {
        uint32_t m_Index;
        HDescriptorSet* m_Set;
    };

    static constexpr auto kBreadcrumbPassID = static_cast<uint32_t>("im_a_pigeon"_crc64);

    class StateVersionBreadcrumbPass : public IDiagnosticPass
    {
    public:
        StateVersionBreadcrumbPass(DeviceDispatchTable* table, DeviceStateTable* state);

        /// Overrides
        void Initialize(VkCommandBuffer cmd_buffer) override;
        void Release() override;
        uint32_t Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage) override;
        void Register(SPIRV::ShaderState *state, spvtools::Optimizer *optimizer) override;
        void Step(VkGPUValidationReportAVA report) override;
        void Report(VkGPUValidationReportAVA report) override;
        void Flush() override;

    public:
        /**
         * Inject breadcrumb data for descriptor bindings
         * @param cmd_buffer the active command buffer
         * @param updates the descriptor set bindings
         * @param count the number of descriptor set bindings
         */
        void BindDescriptorSets(VkCommandBuffer cmd_buffer, const DescriptorSetStateUpdate* updates, uint32_t count);

    private:
        DeviceDispatchTable* m_Table;    ///< The shared device table
        DeviceStateTable*    m_State;    ///< The shared device state
        uint16_t m_BreadcrumbMessageUID; ///< The registry message UID

        ComputeKernel m_BreadcrumbWriteKernel; ///< The kernel used forinlined  breadcrumb writing

        BreadcrumbAllocator<SStateVersionBreadcrumb> m_BreadcrumbAllocator; ///< The shared breadcrumb allocator

    private:
        // Represents a "version" of a descriptor set
        struct SCachedDescriptorData
        {
            std::vector<STrackedWrite> m_Writes;
        };

        std::mutex                          m_BreadcrumbStorageLock;           ///< Shared lock for set storage
        std::vector<SCachedDescriptorData*> m_BreadcrumbDescriptorStorage;     ///< Storage LUT
        std::vector<uint32_t>               m_BreadcrumbDescriptorFreeIndices; ///< All free storage indices
    };
}