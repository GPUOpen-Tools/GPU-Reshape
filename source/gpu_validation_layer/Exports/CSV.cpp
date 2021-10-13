#include <Report.h>
#include <Callbacks.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <sstream>
#include <math.h>

VkResult ExportCSVReport(VkDevice device, VkGPUValidationReportAVA report, const char** out)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));

	std::stringstream ss;
	ss << std::fixed;

	ss << "\"Validation Errors\" Count Type Message Module \"Source Location\" \"Function Name\" \"Source Extract (Estimation)\"\n";

	uint32_t message_count = 0;
	for (const VkGPUValidationMessageAVA& message : report->m_Messages)
	{
		message_count += message.m_MergedCount;

		if (message.m_Type != VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA)
			continue;

		const char* type = "<null>";
		switch (message.m_Error.m_ErrorType)
		{
		default:
			break;
		case VK_GPU_VALIDATION_ERROR_TYPE_IMAGE_OVERFLOW_AVA:
			type = "IMAGE_OVERFLOW_AVA";
			break;
		case VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA:
			type = "BUFFER_OVERFLOW_AVA";
			break;
		case VK_GPU_VALIDATION_ERROR_TYPE_DESCRIPTOR_OVERFLOW_AVA:
			type = "DESCRIPTOR_OVERFLOW_AVA";
			break;
		case VK_GPU_VALIDATION_ERROR_TYPE_EXPORT_UNSTABLE:
			type = "EXPORT_UNSTABLE";
			break;
		case VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA:
			type = "RESOURCE_RACE_CONDITION";
			break;
		}

		ss << " " << message.m_MergedCount << " " << type << " \"" << message.m_Error.m_Message << "\"" << " \"" << message.m_Error.m_SourceExtract.m_Module << "\"";

		// Function name check
		const char* fn_name = message.m_Error.m_SourceExtract.m_Function ? message.m_Error.m_SourceExtract.m_Function : "<NoNameFunction>";

		if (message.m_Error.m_SourceExtract.m_Extract)
		{
			ss << " \"" << message.m_Error.m_SourceExtract.m_File << " ";

			// Begin span
			ss << "[" << message.m_Error.m_SourceExtract.m_Span.m_Begin.m_Line << ":" << message.m_Error.m_SourceExtract.m_Span.m_Begin.m_Character << "]";
			ss << " - ";
			
			// End span
			ss << "[" << message.m_Error.m_SourceExtract.m_Span.m_End.m_Line << ":" << message.m_Error.m_SourceExtract.m_Span.m_End.m_Character << "]\"";

			// Function Extract
			ss << " \"" << fn_name << "\" \"" << message.m_Error.m_SourceExtract.m_Extract << "\"";
		}
		else if (message.m_Error.m_SourceExtract.m_File)
		{
			ss << " \"" << message.m_Error.m_SourceExtract.m_File << "\" \"" << fn_name<< "\" \"<no source information>\"";
		}

		ss << "\n";
	}

	// A bit of a hack due to CSVs limitations
	ss << "Summary\n";
	ss << " \"Recording Time (s)\" \"" << report->m_AccumulatedElapsed << "\"\n";
	ss << " \"Validation Messages\" \"" << message_count << "\"\n";

	if (table->m_CreateInfoAVA.m_LatentTransfers)
	{
		ss << " \"Latent Undershoots\" \"" << report->m_LatentUndershoots << "\" \"" << (report->m_LatentUndershoots / static_cast<float>(report->m_ExportedMessages)) * 100.f << " %\"\n";
		ss << " \"Latent Overshoots\" \"" << report->m_LatentOvershoots << "\" \"" << (report->m_LatentOvershoots / static_cast<float>(report->m_ExportedMessages)) * 100.f << " %\"\n";
	}

	ss << " \"Message Rate (/s)\" \"" << static_cast<uint32_t>(std::ceil(message_count / report->m_AccumulatedElapsed)) << "\"\n";

	report->m_ExportBuffer = ss.str();
	*out = report->m_ExportBuffer.c_str();
	
	return VK_SUCCESS;
}
