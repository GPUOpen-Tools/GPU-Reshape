#pragma once

#include "Common.h"
#include <vector>
#include <chrono>
#include <string>
#include <map>

using TReportTypePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

struct SReportStep
{
	uint64_t m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_COUNT];
	uint64_t m_LatentUndershoots = 0;
	uint64_t m_LatentOvershoots = 0;
};

// The report handle implementation
struct VkGPUValidationReportAVA_T
{
	VkGPUValidationReportBeginInfoAVA m_BeginInfo;

	std::vector<VkGPUValidationMessageAVA> m_Messages;				 ///< All hosted messages
	double								   m_AccumulatedElapsed = 0; ///< The total recording time
	std::string							   m_ExportBuffer;			 ///< The buffer used for export operations

	bool m_IsScheduled = false;

	uint64_t m_ExportedMessages = 0;  ///< Total number of exported messages
	uint64_t m_FilteredMessages = 0;  ///< Total number of filtered messages
	uint64_t m_RecievedMessages = 0;  ///< Total number of recieved messages
	uint64_t m_LatentUndershoots = 0; /// Total number of latent undershoots
	uint64_t m_LatentOvershoots = 0;  /// Total number of latent overshoots

	std::vector< SReportStep> m_Steps;							  ///< The step recordings
	uint64_t				  m_LastSteppedLatentUndershoots = 0; ///< The last recorded number of latent undershoots
	uint64_t				  m_LastSteppedLatentOvershoots = 0;  ///< The last recorded number of latent pversjppts
	TReportTypePoint		  m_LastStepRecord;					  ///< The time point at which the last step recording took place
	double					  m_StepInterval = .25;				  ///< The interval at which a new step is recorded

	uint64_t m_ShaderCompilerCommit = 0;
	uint64_t m_PipelineCompilerCommit = 0;

	TReportTypePoint m_TimeBegin; ///< The time point at which the recording began
};

/* Exposed Report Callbacks */

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationCreateReportAVA(VkDevice device, const VkGPUValidationReportCreateInfoAVA* create_info, VkGPUValidationReportAVA* out);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationDestroyReportAVA(VkDevice device, VkGPUValidationReportAVA report);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationBeginReportAVA(VkDevice device, VkGPUValidationReportAVA report, const VkGPUValidationReportBeginInfoAVA* begin_info);

AVA_C_EXPORT VkGPUValidationReportStatusAVA VKAPI_CALL GPUValidationGetReportStatusAVA(VkDevice device, VkGPUValidationReportAVA report);

AVA_C_EXPORT VkResult VKAPI_PTR GPUValidationDrawDebugAVA(VkDevice device, IDebugRenderer* renderer, const Vectormath::Aos::Vector2& position, const Vectormath::Aos::Vector2& display_size);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationEndReportAVA(VkDevice device);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationPrintReportSummaryAVA(VkDevice device, VkGPUValidationReportAVA report);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationPrintReportAVA(VkDevice device, VkGPUValidationReportAVA report);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationExportReportAVA(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportExportFormat format, const char** out);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationGetReportInfoAVA(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportInfoAVA* out);

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationFlushReportAVA(VkDevice device, VkGPUValidationReportAVA report);

/* Internal Callbacks */

VkResult ExportCSVReport(VkDevice device, VkGPUValidationReportAVA report, const char** out);

VkResult ExportHTMLReport(VkDevice device, VkGPUValidationReportAVA report, const char** out);