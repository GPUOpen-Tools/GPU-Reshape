#pragma once

#include "DiagnosticData.h"
#include "Descriptor.h"
#include "CommandBufferVersion.h"

struct SStringCache;

namespace spvtools { class Optimizer; }
namespace SPIRV { struct ShaderState; }

// Represents a storage request
struct SDiagnosticStorageInfo
{
	uint16_t m_UID		 = UINT16_MAX;
	uint32_t m_EntrySize = UINT32_MAX;
};

// Represents a descriptor request
struct SDiagnosticDescriptorInfo
{
	uint16_t		 m_UID = UINT16_MAX;
	VkDescriptorType m_DescriptorType;
	VkFormat		 m_ElementFormat;
};

// Represents a push constant request
struct SDiagnosticPushConstantInfo
{
	uint16_t m_UID = UINT16_MAX;
	VkFormat m_Format;
};

class IDiagnosticPass
{
public:
	/**
	 * Initialize this pass
	 * @param[in] cmd_buffer the command buffer for data initialization
	 */
	virtual void Initialize(VkCommandBuffer cmd_buffer) = 0;

	/**
	 * Release this pass
	 */
	virtual void Release() = 0;

	/**
	 * Enumerate the storage information
	 * @param[out] storage the outputted storage information, if not null a total of <count> storages will be exported
	 * @param[out] count the number of storages requested
	 */
	virtual void EnumerateStorage(SDiagnosticStorageInfo* storages, uint32_t* count)
	{
		*count = 0;
	}

	/**
	 * Enumerate the descriptor information
	 * @param[out] descriptors the outputted descriptor information, if not null a total of <count> descriptors will be exported
	 * @param[out] count the number of descriptors requested
	 */
	virtual void EnumerateDescriptors(SDiagnosticDescriptorInfo* descriptors, uint32_t* count) 
	{
		*count = 0;
	}

	/**
	 * Enumerate the push constant information
	 * @param[out] constants the outputted push constant information, if not null a total of <count> constants will be exported
	 * @param[out] count the number of descriptors requested
	 */
	virtual void EnumeratePushConstants(SDiagnosticPushConstantInfo* constants, uint32_t* count)
	{
		*count = 0;
	}

	/**
	 * Update any internal push constant data
	 * @param[in] buffer the command buffer in which the constants are uesd
	 * @param[in] constants the push constants that are used, indexed with the allocated push constant uids
	 * @param[in] data the push constant data
	 */
	virtual size_t UpdatePushConstants(VkCommandBuffer buffer, SPushConstantDescriptor* constants, uint8_t* data)
	{
		return 0;
	}

	/**
	 * Create any internal storage on a given descriptor set
	 * @param[in] set the descriptor set handle
	 */
	virtual void CreateDescriptors(HDescriptorSet* set)
	{
		/* */
	}

	/**
	 * Destroy any internal storage on a given descriptor set
	 * @param[in] set the descriptor set handle
	 */
	virtual void DestroyDescriptors(HDescriptorSet* set)
	{
		/* */
	}

	/**
	 * Update a set of descriptors
	 * @param[in] too_descriptors the top level descriptors to be updated
	 * @param[in] diagnostic_descriptors the pass allocated descriptors, indexed using the UIDs
	 * @param[in] top_count the number of top level descriptors requested
	 * @param[in] blob the descriptor blob
	 */
	virtual void UpdateDescriptors(HDescriptorSet* set, bool update, bool push, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob)
	{
		/* */
	}

	/**
	 * Handle a set of messages
	 * @param[in] message_cache the cache to be used for composed string messages
	 * @param[in] version the sequential command buffer version
	 * @param[in] messages the messages to filter
	 * @param[in] count the number of messages in <messages>
	 * @param[in] storage the storage data
	 */
	virtual uint32_t Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage) = 0;

	/**
	 * Register this pass into an optimizer
	 * @param[in] state the shared spirv state
	 * @param[in] optimizer the optimizer to register within
	 */
	virtual void Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer) = 0;

	/**
	 * Step the report
	 * @param[in] report the report where the messages are inserted to
	 */
	virtual void Step(VkGPUValidationReportAVA report) = 0;

	/**
	 * Generate the report
	 * @param[in] report the report where the messages are inserted to
	 */
	virtual void Report(VkGPUValidationReportAVA report) = 0;

	/**
	 * Flush all messages within this pass
	 */
	virtual void Flush() = 0;
};