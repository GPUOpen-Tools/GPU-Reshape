#include <Report.h>
#include <Callbacks.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <CRC.h>
#include <sstream>
#include <cmath>

static const char kHTMLIndexData[] =
{
#	include <gpu_validation/index.html.h>
};

static const char* GetAnonymous(const char* module)
{
	return module ? module : "Anonymous";
}

static uint64_t GetExtractModuleHash(const VkGPUValidationSourceExtractAVA& extract)
{
    uint64_t hash = 0;
    CombineHash(hash, ComputeCRC64(GetAnonymous(extract.m_Module)));
    CombineHash(hash, ComputeCRC64(GetAnonymous(extract.m_ModuleFile)));
    return hash;
}

template<size_t LENGTH>
static void HTMLFormatFeatureBuffer(char(&buffer)[LENGTH], uint32_t feature_set)
{
	buffer[0] = 0;

	uint32_t count = 0;

	// Basic instrumentation
	uint32_t basic_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_BASIC;
	if (basic_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_BASIC)
	{
		FormatBuffer(buffer, count++ ? "%s, Instrumentation Set Basic" : "%sInstrumentation Set Basic", buffer);
	}
	else
	{
		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS)
		FormatBuffer(buffer, count++ ? "%s, Resource Address Bounds" : "%sResource Address Bounds", buffer);

		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY)
			FormatBuffer(buffer, count++ ? "%s, Export Stability" : "%sExport Stability", buffer);

		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS)
			FormatBuffer(buffer, count++ ? "%s, Descriptor Array Bounds" : "%sDescriptor Array Bounds", buffer);
	}

	// Concurrency instrumentation
	uint32_t concurrency_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_CONCURRENCY;
	if (concurrency_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_CONCURRENCY)
	{
		FormatBuffer(buffer, count++ ? "%s, Instrumentation Set Concurrency" : "%sInstrumentation Set Concurrency", buffer);
	}
	else
	{
		if (concurrency_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE)
			FormatBuffer(buffer, count++ ? "%s, Resource Data Race" : "%sResource Data Race", buffer);
	}

	// Data residency instrumentation
	uint32_t dataresidency_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_DATA_RESIDENCY;
	if (dataresidency_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_DATA_RESIDENCY)
	{
		FormatBuffer(buffer, count++ ? "%s, Instrumentation Set Data Residency" : "%sInstrumentation Set Data Residency", buffer);
	}
	else
	{
		if (dataresidency_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)
			FormatBuffer(buffer, count++ ? "%s, Resource Initialization" : "%sResource Initialization", buffer);
	}
}

VkResult ExportHTMLReport(VkDevice device, VkGPUValidationReportAVA report, const char** out)
{
	std::stringstream ss;
	ss << std::fixed;

	ss << "<script>\n";
	
	ss << "Data =\n";
	ss << "{\n";

	// Collect type counts
	uint32_t type_counts[VK_GPU_VALIDATION_ERROR_TYPE_COUNT]{};
	for (const VkGPUValidationMessageAVA& message : report->m_Messages)
	{
		if (message.m_Type != VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA)
			continue;

		type_counts[message.m_Error.m_ErrorType] += message.m_MergedCount;
	}

	// Type lookup table
	uint32_t type_lut[VK_GPU_VALIDATION_ERROR_TYPE_COUNT];

	// Type table
	{
		ss << "\tTypes:\n";
		ss << "\t[\n";
		for (uint32_t c = 0, i = 0; i < VK_GPU_VALIDATION_ERROR_TYPE_COUNT; i++)
		{
			if (!type_counts[i])
				continue;

			const char* name = nullptr;
			switch (i)
			{
                default:
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA:
                    name = "Buffer Overflow";
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_IMAGE_OVERFLOW_AVA:
                    name = "Image Overflow";
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_DESCRIPTOR_OVERFLOW_AVA:
                    name = "Descriptor Overflow";
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_EXPORT_UNSTABLE:
                    name = "Export Unstable";
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA:
                    name = "Race Condition";
                    break;
                case VK_GPU_VALIDATION_ERROR_TYPE_SUBRESOURCE_UNINITIALIZED:
                    name = "Subresource Uninitialized";
                    break;
			}
			ss << "\t\t{ Name: \"" << name << "\" },\n";

			// Assign lookup index
			type_lut[i] = c++;
		}
		ss << "\t],\n\n";
	}

	// Summary table
	{
		ss << "\tSummary:\n";
		ss << "\t{\n";

		// Latency table
		{
			ss << "\t\tLatency:\n";
			ss << "\t\t{\n";
			ss << "\t\t\tOvershoots: " << report->m_LatentOvershoots << ",\n";
			ss << "\t\t\tUndershoots: " << report->m_LatentUndershoots << ",\n";
			ss << "\t\t},\n";
		}

		// Count table
		{
			ss << "\t\tCounts:\n";
			ss << "\t\t[\n";

			for (uint32_t i = 0; i < VK_GPU_VALIDATION_ERROR_TYPE_COUNT; i++)
			{
				if (!type_counts[i])
					continue;

				ss << "\t\t\t{ Type: " << type_lut[i] << ", Count: " << type_counts[i] << " },\n";
			}

			ss << "\t\t]\n";
		}

		ss << "\t},\n\n";
	}

	// Time sliced table
	{
		ss << "\tTimeSliced:\n";
		ss << "\t[\n";

		double label = 0;
		for (const SReportStep& step : report->m_Steps)
		{
			ss << "\t\t{\n";

			ss << "\t\t\tLabel: " << label << ",\n";

			ss << "\t\t\tLatency:\n";
			ss << "\t\t\t{\n";
			ss << "\t\t\t\tOvershoots: " << step.m_LatentOvershoots << ",\n";
			ss << "\t\t\t\tUndershoots: " << step.m_LatentUndershoots << ",\n";
			ss << "\t\t\t},\n";

			ss << "\t\t\tCounts:\n";
			ss << "\t\t\t[\n";
			for (uint32_t i = 0; i < VK_GPU_VALIDATION_ERROR_TYPE_COUNT; i++)
			{
				if (!type_counts[i])
					continue;

				ss << "\t\t\t\t{ Type: " << type_lut[i] << ", Count: " << step.m_ErrorCounts[i] << " },\n";
			}
			ss << "\t\t\t]\n";

			ss << "\t\t},\n";
			label += report->m_StepInterval;
		}

		ss << "\t], \n\n";
	}

	struct Module
    {
        const char* m_Module;
        const char* m_ModuleFile;
    };

	// Get all unique modules
	std::unordered_map<uint64_t, Module> modules;
	for (const VkGPUValidationMessageAVA& message : report->m_Messages)
	{
		if (message.m_Type != VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA)
			continue;

		uint64_t hash = GetExtractModuleHash(message.m_Error.m_SourceExtract);

		Module module{};
        module.m_Module = message.m_Error.m_SourceExtract.m_Module;
        module.m_ModuleFile = message.m_Error.m_SourceExtract.m_ModuleFile;
		modules.insert(std::make_pair(hash, module));
	}

	// Shader table
	{
		ss << "\tShaders:\n";
		ss << "\t[\n";
		for (const auto& kv : modules)
		{
			ss << "\t\t{\n";
            ss << "\t\t\tModule: \"" << kv.second.m_Module << "\",\n";
            ss << "\t\t\tModuleFile: \"" << kv.second.m_ModuleFile << "\",\n";

			uint32_t error_count = 0;
			uint32_t unique_error_count = 0;
			uint32_t module_feature_mask = 0;

			// Message table
			{
				ss << "\t\t\tMessages:\n";
				ss << "\t\t\t[\n";
				for (const VkGPUValidationMessageAVA& message : report->m_Messages)
				{
					if (message.m_Type != VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA)
						continue;

                    uint64_t hash = GetExtractModuleHash(message.m_Error.m_SourceExtract);
					if (kv.first != hash)
						continue;

					module_feature_mask |= message.m_Feature;

					unique_error_count++;
					error_count += message.m_MergedCount;

					ss << "\t\t\t\t{\n";

					ss << "\t\t\t\t\tType: " << type_lut[message.m_Error.m_ErrorType] << ",\n";

					if (message.m_Error.m_Message)
						ss << "\t\t\t\t\tMessage: \"" << message.m_Error.m_Message << "\",\n";
					else
						ss << "\t\t\t\t\tMessage: \"NoMessage\",\n";

					ss << "\t\t\t\t\tCount: " << message.m_MergedCount << ",\n";

					if (message.m_Error.m_ObjectInfo.m_Name)
                        ss << "\t\t\t\t\tObject: \"" << message.m_Error.m_ObjectInfo.m_Name << "\",\n";
					else
                        ss << "\t\t\t\t\tObject: \"NoName\",\n";

					ss << "\t\t\t\t\tLocation:\n";
					ss << "\t\t\t\t\t{\n";
					ss << "\t\t\t\t\t\tFile: \"" << GetAnonymous(message.m_Error.m_SourceExtract.m_File) << "\",\n";
					if (message.m_Error.m_SourceExtract.m_Extract)
					{
						ss << "\t\t\t\t\t\tExtract: \"" << message.m_Error.m_SourceExtract.m_Extract << "\",\n";
						ss << "\t\t\t\t\t\tLine: " << message.m_Error.m_SourceExtract.m_Span.m_Begin.m_Line << ",\n";
						ss << "\t\t\t\t\t\tColumn: " << message.m_Error.m_SourceExtract.m_Span.m_Begin.m_Character << ",\n";
					}
					else
					{
						ss << "\t\t\t\t\t\tExtract: \"<Failed to extract>\",\n";
						ss << "\t\t\t\t\t\tLine: 0,\n";
						ss << "\t\t\t\t\t\tColumn: 0,\n";
					}
					if (message.m_Error.m_SourceExtract.m_Function)
					{
						ss << "\t\t\t\t\t\tFunction: \"" << message.m_Error.m_SourceExtract.m_Function << "\",\n";
					}
					else
					{
						ss << "\t\t\t\t\t\tFunction: \"<Failed to extract>\",\n";
					}
					ss << "\t\t\t\t\t}\n";

					ss << "\t\t\t\t},\n";
				}
				ss << "\t\t\t],\n";
			}

			char feature_buffer[512];
			HTMLFormatFeatureBuffer(feature_buffer, module_feature_mask);

			ss << "\t\t\tErrors: " << error_count << ",\n";
			ss << "\t\t\tUniqueErrors: " << unique_error_count << ",\n";
			ss << "\t\t\tFeatures: \"" << feature_buffer << "\",\n";

			ss << "\t\t},\n";
		}
		ss << "\t],\n";
	}

	ss << "};";
	
	ss << "</script>\n";

	ss << kHTMLIndexData;

	report->m_ExportBuffer = ss.str();
	*out = report->m_ExportBuffer.c_str();
	
	return VK_SUCCESS;
}
