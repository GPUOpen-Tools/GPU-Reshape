#include <Report.h>
#include <Callbacks.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <ShaderCompiler.h>
#include <PipelineCompiler.h>
#include <DiagnosticAllocator.h>
#include <DiagnosticRegistry.h>
#include <sstream>
#include <cmath>

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationCreateReportAVA(VkDevice device, const VkGPUValidationReportCreateInfoAVA * create_info, VkGPUValidationReportAVA * out)
{
	auto report = new VkGPUValidationReportAVA_T();

	// Wow, so much work done, truly next gen
	*out = report;
	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationDestroyReportAVA(VkDevice device, VkGPUValidationReportAVA report)
{
	auto report_t = reinterpret_cast<VkGPUValidationReportAVA_T*>(report);
	delete report_t;

	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationBeginReportAVA(VkDevice device, VkGPUValidationReportAVA report, const VkGPUValidationReportBeginInfoAVA* begin_info)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Report operations must be sync
	std::lock_guard<std::mutex> report_guard(device_state->m_ReportLock);

	// Must not already be recording
	VkResult result;
	if (device_state->m_ActiveReport)
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Another report is still being recorded, only a single report may recording at a time");
		}

		return VK_NOT_READY;
	}

	// Initialize report
	report->m_BeginInfo = *begin_info;
	report->m_TimeBegin = std::chrono::high_resolution_clock::now();
	report->m_LastStepRecord = std::chrono::high_resolution_clock::now();
	report->m_IsScheduled = true;

	// Wait for all previous GPU commands to finish for recompilation of active in flight states
	table->m_DeviceWaitIdle(device);

	// Instrument all descriptors (sync)
	// Parallelising this would be bat shit insane, pay me more
	if ((result = InstrumentDescriptors(device, report)) != VK_SUCCESS)
	{
		return result;
	}

	// Instrument all pipelines (async)
	if ((result = InstrumentPipelines(device, report)) != VK_SUCCESS)
	{
		return result;
	}

	// Requires wait?
	if (begin_info->m_WaitForCompilation)
	{
		// Spin me right round
		while (!device_state->m_ShaderCompiler->IsCommitPushed(report->m_ShaderCompilerCommit) || !device_state->m_PipelineCompiler->IsCommitPushed(report->m_PipelineCompilerCommit));
	}

	device_state->m_ActiveReport = report;
	return VK_SUCCESS;
}

AVA_C_EXPORT VkGPUValidationReportStatusAVA VKAPI_CALL GPUValidationGetReportStatusAVA(VkDevice device, VkGPUValidationReportAVA report)
{
	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Is it scheduled for recording?
	if (!report->m_IsScheduled)
	{
		VkGPUValidationReportStatusAVA status;
		status.m_Type = VK_GPU_VALIDATION_REPORT_STATUS_IDLE;
		return status;
	}

	// May still be pending
	if (!device_state->m_ShaderCompiler->IsCommitPushed(report->m_ShaderCompilerCommit))
	{
		VkGPUValidationReportStatusAVA status;
		status.m_Type = VK_GPU_VALIDATION_REPORT_STATUS_PENDING_SHADER_COMPILATION;
		status.m_PendingShaders = device_state->m_ShaderCompiler->GetPendingCommits(report->m_ShaderCompilerCommit);
		return status;
	}

	// May still be pending
	if (!device_state->m_PipelineCompiler->IsCommitPushed(report->m_PipelineCompilerCommit))
	{
		VkGPUValidationReportStatusAVA status;
		status.m_Type = VK_GPU_VALIDATION_REPORT_STATUS_PENDING_PIPELINE_COMPILATION;
		status.m_PendingPipelines = device_state->m_PipelineCompiler->GetPendingCommits(report->m_PipelineCompilerCommit);
		return status;
	}

	// Actively recording
	VkGPUValidationReportStatusAVA status;
	status.m_Type = VK_GPU_VALIDATION_REPORT_STATUS_RECORDING;
	return status;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationEndReportAVA(VkDevice device)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Report operations must be sync
	std::lock_guard<std::mutex> report_gyuard(device_state->m_ReportLock);

	// Must be recording
	if (!device_state->m_ActiveReport)
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Attempting to stop recording when none is active");
		}

		return VK_NOT_READY;
	}

	// Accumulate recording time
	device_state->m_ActiveReport->m_AccumulatedElapsed += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - device_state->m_ActiveReport->m_TimeBegin).count() * 1e-9;
	device_state->m_ActiveReport->m_IsScheduled = false;

	// Finish all gpu operations
	VkResult result = table->m_DeviceWaitIdle(device);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Push all pending allocations
	device_state->m_DiagnosticAllocator->WaitForPendingAllocations();

	// Wait for filtering
	device_state->m_DiagnosticAllocator->WaitForFiltering();

	// Generate report
	device_state->m_DiagnosticRegistry->GenerateReport(device_state->m_ActiveReport);
	device_state->m_DiagnosticRegistry->Flush();

	// No longer active
	device_state->m_ActiveReport = nullptr;
	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationPrintReportSummaryAVA(VkDevice device, VkGPUValidationReportAVA report)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	uint32_t message_count = 0;
	for (const VkGPUValidationMessageAVA& msg : report->m_Messages)
	{
		message_count += msg.m_MergedCount;
	}

	std::stringstream ss;
	ss << std::fixed;
	ss << "GPU Validation Report Summary\n";
	ss << "\t      Recording Time : " << report->m_AccumulatedElapsed << "s\n";
	ss << "\t Validation Messages : " << message_count << "\n";
	
	if (table->m_CreateInfoAVA.m_LatentTransfers)
	{
		//ss << "\t   Exported Messages : " << report->m_ExportedMessages << "\n";
		//ss << "\t   Filtered Messages : " << report->m_FilteredMessages << "\n";
		ss << "\t  Latent Undershoots : " << report->m_LatentUndershoots << " (" << (report->m_LatentUndershoots / static_cast<float>(report->m_ExportedMessages)) * 100.f << "%)\n";
		ss << "\t   Latent Overshoots : " << report->m_LatentOvershoots << " (" << (report->m_LatentOvershoots / static_cast<float>(report->m_ExportedMessages)) * 100.f << "%)\n";
	}

	ss << "\t        Message Rate : " << static_cast<uint32_t>(std::ceil(message_count / report->m_AccumulatedElapsed)) << " /s\n";

	// TODO: Dump to output stream if null?
	if (table->m_CreateInfoAVA.m_MessageCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, ss.str().c_str());
	}

	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationPrintReportAVA(VkDevice device, VkGPUValidationReportAVA report)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// TODO: Dump to output stream if null?
	if (table->m_CreateInfoAVA.m_MessageCallback)
	{
		VkGPUValidationReportInfoAVA info;
		info.m_Report = report;
		info.m_Messages = report->m_Messages.data();
		info.m_MessageCount = static_cast<uint32_t>(report->m_Messages.size());
		table->m_CreateInfoAVA.m_MessageCallback(table->m_CreateInfoAVA.m_UserData, &info);
	}

	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationExportReportAVA(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportExportFormat format, const char** out)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Report operations must be sync
	std::lock_guard<std::mutex> report_gyuard(device_state->m_ReportLock);

	switch (format)
	{
		default:
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Unsupported report export format");
			}
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}
		case VK_GPU_VALIDATION_REPORT_EXPORT_FORMAT_CSV:
			return ExportCSVReport(device, report, out);
		case VK_GPU_VALIDATION_REPORT_EXPORT_FORMAT_HTML:
			return ExportHTMLReport(device, report, out);
	}
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationGetReportInfoAVA(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportInfoAVA * out)
{
	out->m_Report = report;
	out->m_Messages = report->m_Messages.data();
	out->m_MessageCount = static_cast<uint32_t>(report->m_Messages.size());
	return VK_SUCCESS;
}

AVA_C_EXPORT VkResult VKAPI_CALL GPUValidationFlushReportAVA(VkDevice device, VkGPUValidationReportAVA report)
{
	report->m_Messages.clear();
	report->m_ExportedMessages = 0;
	report->m_FilteredMessages = 0;
	report->m_RecievedMessages = 0;
	report->m_LatentUndershoots = 0;
	report->m_LatentOvershoots = 0;
	report->m_LastSteppedLatentOvershoots = 0;
	report->m_LastSteppedLatentUndershoots = 0;
	report->m_AccumulatedElapsed = 0;
	report->m_ExportBuffer.clear();
	report->m_Steps.clear();
	return VK_SUCCESS;
}
