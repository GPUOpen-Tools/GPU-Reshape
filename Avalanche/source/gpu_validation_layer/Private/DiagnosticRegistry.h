#pragma once

#include "DiagnosticPass.h"
#include "ShaderLocationRegistry.h"
#include <StringCache.h>

class DiagnosticRegistry
{
public:
	DiagnosticRegistry();

	/**
	 * Initialize this registry
	 * @param[in] create_info the creation info
	 */
	void Initialize(const VkGPUValidationCreateInfoAVA& create_info);

	/**
	 * Release this registry
	 */
	void Release();

	/**
	 * Initialize all passes within this registry
	 * @param[in] cmd_buffer the command buffer for data initialization
	 */
	void InitializePasses(VkCommandBuffer cmd_buffer);

	/**
	 * Register a pass within this registry
	 * @param[out] feature_id the feature id at which this pass is used
	 * @param[out] pass the pass to be registered
	 */
	void Register(uint32_t feature_id, IDiagnosticPass* pass);

	/**
	 * Get a registered pass
	 * @param[out] active_features the currently active feature mask
	 * @param[out] feature_id the feature id at which the pass is used
	 */
	IDiagnosticPass* GetPass(uint32_t active_features, uint32_t feature_id);

	/**
	 * Enumerate the storage information
	 * @param[out] storage the outputted storage information, if not null a total of <count> storages will be exported
	 * @param[out] count the number of storages requested
	 */
	void EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count);

	/**
	 * Enumerate the descriptor information
	 * @param[out] descriptors the outputted descriptor information, if not null a total of <count> descriptors will be exported
	 * @param[out] count the number of descriptors requested
	 */
	void EnumerateDescriptors(SDiagnosticDescriptorInfo* descriptors, uint32_t* count);

	/**
	 * Enumerate the push constant information
	 * @param[out] constants the outputted push constant information, if not null a total of <count> constants will be exported
	 * @param[out] count the number of descriptors requested
	 */
	void EnumeratePushConstants(SDiagnosticPushConstantInfo* constants, uint32_t* count);

	/**
	 * Update any internal push constant data
	 * @param[in] buffer the command buffer in which the constants are uesd
	 * @param[in] constants the push constants that are used, indexed with the allocated push constant uids
	 * @param[in] data the push constant data
	 */
	size_t UpdatePushConstants(VkCommandBuffer buffer, uint32_t feature_set, SPushConstantDescriptor* constants, uint8_t* data);

	/**
	 * Create any internal storage on a given descriptor set
	 * @param[in] set the descriptor set handle
	 */
	void CreateDescriptors(HDescriptorSet* set);

	/**
	 * Destroy any internal storage on a given descriptor set
	 * @param[in] set the descriptor set handle
	 */
	void DestroyDescriptors(HDescriptorSet* set);

	/**
	 * Update a set of descriptors
	 * @param[in] set the descriptor set handle
	 * @param[in] too_descriptors the top level descriptors to be updated
	 * @param[in] diagnostic_descriptors the pass allocated descriptors, indexed using the UIDs
	 * @param[in] top_count the number of top level descriptors requested
	 * @param[in] blob the descriptor blob
	 */
	void UpdateDescriptors(HDescriptorSet* set, bool push, uint32_t feature_set, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob);

	/**
	 * Handle a complete diagnostics allocations data
	 * @param[in] version the sequential command buffer version
	 * @param[in] data the complete diagnostics data
	 * @param[in] storage the storage data
	 */
	uint32_t Handle(SCommandBufferVersion& version, const SDiagnosticData* data, void* const* storage);

	/**
	 * Register all passes into an optimizer
	 * @param[in] state the shared spirv state
	 * @param[in] optimizer the optimizer to register within
	 */
	void Register(uint32_t feature_mask, SPIRV::ShaderState* state, spvtools::Optimizer* optimizer);

	/**
	 * Generate a report
	 * @param[in] report the report to be generated
	 */
	void GenerateReport(VkGPUValidationReportAVA report);

	/**
	 * Step a report
	 * @param[in] report the report to be generated
	 */
	void StepReport(VkGPUValidationReportAVA report);

	/**
	 * Flush all messages
	 */
	void Flush();

	/**
	 * Get the feature mask version identifier
	 * @param[in] feature_mask the set of features to be used
	 */
	uint64_t GetFeatureVersionUID(uint32_t feature_mask);

	/**
	 * Allocate a new message identifier
	 */
	uint16_t AllocateMessageUID();

	/**
	 * Allocate a new storage identifier
	 */
	uint16_t AllocateStorageUID();

	/**
	 * Allocate a new descriptor identifier
	 */
	uint16_t AllocateDescriptorUID();

	/**
	 * Allocate a new descriptor storage identifier
	 */
	uint16_t AllocateDescriptorStorageUID();

	/**
	 * Allocate a new push constant identifier
	 */
	uint16_t AllocatePushConstantUID();

	/**
	 * Set the handler to a message identifier
	 * @param[in] uid the identifier
	 * @param[in] handler the handler to be registered to <uid>
	 */
	void SetMessageHandler(uint16_t uid, IDiagnosticPass* handler);

	/**
	 * Get the location registry
	 */
	ShaderLocationRegistry* GetLocationRegistry() 
	{
		return &m_LocationRegistry;
	}

	/**
	 * Get the total number of messages allocated
	 */
	uint16_t GetAllocatedMessageUIDs() const
	{ 
		return m_MessageUID; 
	}

	/**
	 * Get the total number of storages allocated
	 */
	uint16_t GetAllocatedStorageUIDs() const
	{
		return m_StorageUID;
	}

	/**
	 * Get the total number of descriptors allocated
	 */
	uint16_t GetAllocatedDescriptorUIDs() const
	{
		return m_DescriptorUID;
	}

	/**
	 * Get the total number of descriptors allocated
	 */
	uint16_t GetAllocatedDescriptorStorageUIDs() const
	{
		return m_DescriptorStorageUID;
	}

	/**
	 * Get the total number of descriptors allocated
	 */
	uint16_t GetAllocatedPushConstantUIDs() const
	{
		return m_PushConstantUID;
	}

private:
	// No copy
	DiagnosticRegistry(const DiagnosticRegistry&) = delete;
	DiagnosticRegistry& operator=(const DiagnosticRegistry&) = delete;

	// Represents a registered pass
	struct PassInfo
	{
		IDiagnosticPass* m_Pass;
		uint64_t m_FeatureID;
	};
	
	IDiagnosticPass*			  m_LUT[kMaxMessageTypes]{};  ///< Pass lookup table
	std::vector<PassInfo>		  m_Passes;					  ///< All registered passes
	uint16_t					  m_MessageUID			 = 0; ///< Message identifier head
	uint16_t					  m_StorageUID			 = 0; ///< Storage identifier head
	uint16_t					  m_DescriptorUID        = 0; ///< Descriptor identifier head
	uint16_t					  m_DescriptorStorageUID = 0; ///< Descriptor storage identifier head
	uint16_t					  m_PushConstantUID      = 0; ///< Push constant identifier head
	ShaderLocationRegistry		  m_LocationRegistry;		  ///< Internally hosted location registry
	std::mutex					  m_FilterLock;				  ///< Filtering lock
    SStringCache                  m_StringCache;              ///< Message string cache
};