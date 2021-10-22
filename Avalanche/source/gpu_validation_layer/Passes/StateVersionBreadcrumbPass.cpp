//
// Created by mipe on 18/09/2019.
//

#include <Passes/StateVersionBreadcrumbPass.h>
#include <CommandBufferVersion.h>
#include <cstring>

using namespace Passes;

// Shared Kernel Data
#include <Private/StateTables.h>
#include <Private/Passes/StateVersionBreadcrumbPass.h>

// Breadcrumb Write Kernel
static constexpr uint8_t kKernelBreadcrumbWrite[] =
{
#	include <gpu_validation/Passes/BreadcrumbWrite.spirv.h>
};

static constexpr uint32_t kBreadcrumbWriteBatchSize = 4;

struct BreadcrumbWriteData
{
    uint32_t m_PackedMessages32[kBreadcrumbWriteBatchSize]{};
};

struct SStateVersionBreadcrumbMessage
{
    uint32_t m_BreadcrumbID : 25; // 1
    uint32_t m_ZeroGuard    : 1;  // 0
};

StateVersionBreadcrumbPass::StateVersionBreadcrumbPass(DeviceDispatchTable* table, DeviceStateTable* state) : m_Table(table), m_State(state)
{
    m_BreadcrumbMessageUID = state->m_DiagnosticRegistry->AllocateMessageUID();

    state->m_DiagnosticRegistry->SetMessageHandler(m_BreadcrumbMessageUID, this);
}

void StateVersionBreadcrumbPass::Initialize(VkCommandBuffer cmd_buffer)
{
    // Create breadcrumb write kernel
    {
        VkDescriptorType descriptor_types[] =
        {
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER /* Diagnostic Data */
        };

        ComputeKernelInfo info;
        info.Kernel(kKernelBreadcrumbWrite);
        info.DescriptorTypes(descriptor_types);
        info.m_PCByteSpan = sizeof(BreadcrumbWriteData);
        m_BreadcrumbWriteKernel.Initialize(m_Table->m_Device, info);
    }
}

void StateVersionBreadcrumbPass::Release()
{
    // Release all storages
    for (SCachedDescriptorData* data : m_BreadcrumbDescriptorStorage)
    {
        delete data;
    }

    m_BreadcrumbWriteKernel.Destroy();
}

uint32_t StateVersionBreadcrumbPass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *)
{
    std::lock_guard<std::mutex> guard(m_BreadcrumbStorageLock);

    for (uint32_t i = 0; i < count; i++)
    {
        // Translate message
        auto message = messages[i].GetMessage<SStateVersionBreadcrumbMessage>();

        // Get breadcrumb
        BreadcrumbAllocation<SStateVersionBreadcrumb> breadcrumb = m_BreadcrumbAllocator.GetAllocation(BreadcrumbID(message.m_BreadcrumbID));

        // Handle it
        switch (breadcrumb->m_Type)
        {
            default:
            {
                if (m_Table->m_CreateInfoAVA.m_LogCallback && (m_Table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
                {
                    m_Table->m_CreateInfoAVA.m_LogCallback(m_Table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Corrupt breadcrumb data");
                }
                break;
            }
            case EStateVersionBreadcrumbType::eDescriptorSet:
            {
                SCachedDescriptorData* cached = m_BreadcrumbDescriptorStorage[breadcrumb->m_DescriptorSet.m_StorageIndex];

                // Accept changes
                version.GetDescriptorSet(breadcrumb->m_DescriptorSet.m_SetIndex).Accept(cached->m_Writes);

                // Free cached set
                m_BreadcrumbDescriptorFreeIndices.push_back(breadcrumb->m_DescriptorSet.m_StorageIndex);
                break;
            }
        }

        // Mark as free'd
        m_BreadcrumbAllocator.FreeAllocation(breadcrumb);
    }

    return count;
}

void StateVersionBreadcrumbPass::Register(SPIRV::ShaderState *state, spvtools::Optimizer *optimizer)
{
    /* No SPIRV injection is required for breadcrumb support */
}

void StateVersionBreadcrumbPass::Step(VkGPUValidationReportAVA report)
{
    /* poof */
}

void StateVersionBreadcrumbPass::Report(VkGPUValidationReportAVA report)
{
    /* poof */
}

void StateVersionBreadcrumbPass::Flush()
{
    /* poof */
}

void StateVersionBreadcrumbPass::BindDescriptorSets(VkCommandBuffer cmd_buffer, const DescriptorSetStateUpdate* updates, uint32_t count)
{
    CommandStateTable* cmd_state = CommandStateTable::Get(cmd_buffer);

    // Must be serial
    std::lock_guard<std::mutex> guard(m_BreadcrumbStorageLock);

    // Create allocations
    auto breadcrumbs = ALLOCA_ARRAY(BreadcrumbAllocation<SStateVersionBreadcrumb>, count);
    for (uint32_t i = 0; i < count; i++)
    {
        auto&& bc = breadcrumbs[i];

        bc = m_BreadcrumbAllocator.PopAllocation();
        bc->m_Type = EStateVersionBreadcrumbType::eDescriptorSet;
        bc->m_DescriptorSet.m_SetIndex = updates[i].m_Index;

        // Allocate storage
        uint32_t storage;
        if (m_BreadcrumbDescriptorFreeIndices.empty())
        {
            /* No free storage */
            storage = static_cast<uint32_t>(m_BreadcrumbDescriptorStorage.size());
            m_BreadcrumbDescriptorStorage.push_back(new SCachedDescriptorData());
        }
        else
        {
            /* Free storage available */
            storage = m_BreadcrumbDescriptorFreeIndices.back();
            m_BreadcrumbDescriptorFreeIndices.pop_back();
        }

        // Copy data
        m_BreadcrumbDescriptorStorage[storage]->m_Writes.assign(updates[i].m_Set->m_TrackedWrites.begin(), updates[i].m_Set->m_TrackedWrites.end());

        // Write breadcrumb data
        bc->m_DescriptorSet.m_StorageIndex = storage;
    }

    // Messages are written to in batches
    for (uint32_t i = 0; i < count; i += kBreadcrumbWriteBatchSize)
    {
        BreadcrumbWriteData data;
        std::memset(&data, 0, sizeof(data));

        // Compose the messages
        for (uint32_t j = 0; j < std::min(kBreadcrumbWriteBatchSize, count - i); j++)
        {
            SStateVersionBreadcrumbMessage message{};
            message.m_BreadcrumbID = static_cast<uint32_t>(breadcrumbs[i + j].m_Identifier.m_ID);
            message.m_ZeroGuard = 1;

            // Insert key
            data.m_PackedMessages32[j] = SDiagnosticMessageData::Construct(m_BreadcrumbMessageUID, message).GetKey();
        }

        // Atomically insert the messages
        m_BreadcrumbWriteKernel.Dispatch(cmd_buffer, cmd_state->m_Allocation->m_DescriptorSet, data);
    }
}

