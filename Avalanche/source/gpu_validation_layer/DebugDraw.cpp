#include <Report.h>
#include <Callbacks.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <ShaderCompiler.h>
#include <PipelineCompiler.h>
#include <DiagnosticData.h>
#include <sstream>
#include <cmath>

#include <DebugRender/IDebugRenderer.h>

template<size_t LENGTH>
static void DebugDrawFormatFeatureBuffer(char(&buffer)[LENGTH], uint32_t feature_set)
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

// Helper context for drawing
struct SDebugDrawContext
{
	IDebugRenderer* m_Renderer;
	AosVector2		m_PixelFactor;
	AosPoint2		m_Position;
	float			m_FontSize;

	AosVector2		m_TablePosition;
	AosVector2		m_TablePadding;
	
	uint32_t		m_Columns;
	uint32_t		m_Rows;
	float			m_ColumnOffsets[16];
	float			m_RowHeight;
};

template<uint32_t LENGTH>
static AosVector2 ConfigureTable(SDebugDrawContext& context, const AosVector2& position, float width, const float (&column_widths)[LENGTH], uint32_t rows)
{
	context.m_Columns = LENGTH;
	context.m_Rows = rows;

	// Get total width
	float total = 0;
	for (uint32_t i = 0; i < LENGTH; i++)
	{
		total += column_widths[i];
	}

	// Account for required proportions
	float offset = 0;
	for (uint32_t i = 0; i < LENGTH; i++)
	{
		context.m_ColumnOffsets[i] = offset * (width / total) * context.m_PixelFactor.getX();

		offset += column_widths[i];
	}

	float row_height = 25;

	// Default padding
	AosVector2 padding{ 15, 15 };

	// Determine extends, account for padding
	AosVector2 extents = padding * 2 + AosVector2{ width, row_height * rows };

	// Conver to relative coordinates
	context.m_RowHeight = context.m_PixelFactor.getY() * row_height;
	context.m_TablePadding = Vectormath::Aos::mulPerElem(padding, context.m_PixelFactor);
	context.m_TablePosition = Vectormath::Aos::mulPerElem(position, context.m_PixelFactor);

	// Background color
	//context.m_Renderer->DebugRectFilled(
	//	context.m_Position +	context.m_TablePosition,
	//	context.m_Position + Vectormath::Aos::mulPerElem(position + extents, context.m_PixelFactor),
	//	SColorRGBAb{ 0, 0, 0, static_cast<uint8_t>(255 / 1.5f) }
	//);

	return extents;
}

static AosPoint2 TablePos(SDebugDrawContext& context, uint32_t row, uint32_t column)
{
	return context.m_Position + context.m_TablePadding + context.m_TablePosition + AosVector2{ context.m_ColumnOffsets[column], context.m_RowHeight * row };
}

template<typename... T>
static void DrawStr(SDebugDrawContext& context, const AosPoint2& pos, const char* format, T&&... args)
{
	//context.m_Renderer->DebugTextFmt(pos, SColorRGBAb{ 255, 255, 255, 255 }, true, context.m_FontSize, format, args...);
}

static uint64_t PrettyCount(uint64_t n, const char** postfix)
{
	if (n >= 1e11)
	{
		*postfix = "b";
		return n / static_cast<uint64_t>(1e9);
	}

	if (n >= 1e8)
	{
		*postfix = "m";
		return n / static_cast<uint64_t>(1e6);
	}

	if (n >= 1e5)
	{
		*postfix = "k";
		return n / static_cast<uint64_t>(1e3);
	}

	*postfix = "";
	return n;
}

static uint64_t PrettySize(uint64_t n, const char** postfix)
{
	if (n >= 1e10)
	{
		*postfix = "gb";
		return n / static_cast<uint64_t>(1e9);
	}

	if (n >= 1e7)
	{
		*postfix = "mb";
		return n / static_cast<uint64_t>(1e6);
	}

	if (n >= 1e4)
	{
		*postfix = "kb";
		return n / static_cast<uint64_t>(1e3);
	}

	*postfix = "";
	return n;
}

AVA_C_EXPORT VkResult VKAPI_PTR GPUValidationDrawDebugAVA(VkDevice device, IDebugRenderer* renderer, const AosVector2& position, const AosVector2& display_size)
{
	// Get states
	DeviceStateTable* device_state = DeviceStateTable::Get(GetKey(device));

	// Prepare context
	SDebugDrawContext context;
	context.m_Renderer = renderer;
	context.m_PixelFactor = Vectormath::Aos::recipPerElem(display_size);
	context.m_Position = AosPoint2(position);
	context.m_FontSize = .75f;

	// Report operations must be sync
	std::lock_guard<std::mutex> report_guard(device_state->m_ReportLock);
	auto report = device_state->m_ActiveReport;

	// Main table
	{
		char feature_buffer[256];
		DebugDrawFormatFeatureBuffer(feature_buffer, report->m_BeginInfo.m_Features);

		// Compiling?
		if (!device_state->m_ShaderCompiler->IsCommitPushed(report->m_ShaderCompilerCommit) || !device_state->m_PipelineCompiler->IsCommitPushed(report->m_PipelineCompilerCommit))
		{
			/*  { 4, 2 }
				GPU Validation    : Compiling
				Features          : s

				Pending Shaders   : #
				Pending Pipelines : #
			*/

			float widths[] = { 100, 85 };
			ConfigureTable(context, AosVector2 { 0, 0 }, 500, widths, 5);

			DrawStr(context, TablePos(context, 0, 0), "GPU Validation");
			DrawStr(context, TablePos(context, 0, 1), "Compiling");

			DrawStr(context, TablePos(context, 1, 0), "Features");
			DrawStr(context, TablePos(context, 1, 1), "%s", feature_buffer);

			DrawStr(context, TablePos(context, 3, 0), "Pending Shaders");
			DrawStr(context, TablePos(context, 3, 1), "%i", device_state->m_ShaderCompiler->GetPendingCommits(report->m_ShaderCompilerCommit));

			DrawStr(context, TablePos(context, 4, 0), "Pending Pipelines");
			DrawStr(context, TablePos(context, 4, 1), "%i", device_state->m_PipelineCompiler->GetPendingCommits(report->m_PipelineCompilerCommit));
		}
		else
		{
			// Overview
			AosVector2 overview_extents;
			{
				/*  { , 2 }
					GPU Validation       : Recording
					Features			 : s

					Time Elapsed         : #s
					Validation Errors    : #
					Latent Undershoots   : #%
					Latent Overshoots    : #%
					Transferred Data     : #
				*/

				float widths[] = { 100, 85 };
				overview_extents = ConfigureTable(context, AosVector2{ 0, 0 }, 500, widths, 8);

				DrawStr(context, TablePos(context, 0, 0), "GPU Validation");
				DrawStr(context, TablePos(context, 0, 1), "Recording");

				DrawStr(context, TablePos(context, 1, 0), "Features");
				DrawStr(context, TablePos(context, 1, 1), "%s", feature_buffer);

				double elapsed = report->m_AccumulatedElapsed + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - device_state->m_ActiveReport->m_TimeBegin).count() * 1e-9;

				DrawStr(context, TablePos(context, 3, 0), "Time Elapsed");
				DrawStr(context, TablePos(context, 3, 1), "%is", static_cast<uint32_t>(elapsed));

				{
					const char* postfix = nullptr;
					uint64_t pretty_message_count = PrettyCount(report->m_FilteredMessages, &postfix);

					DrawStr(context, TablePos(context, 4, 0), "Validation Errors");
					DrawStr(context, TablePos(context, 4, 1), "%lu%s", pretty_message_count, postfix);
				}

				{
					DrawStr(context, TablePos(context, 5, 0), "Latent Undershoots");
					DrawStr(context, TablePos(context, 5, 1), "%i%%", static_cast<uint32_t>((report->m_LatentUndershoots / static_cast<float>(report->m_FilteredMessages)) * 100));
				}

				{
					DrawStr(context, TablePos(context, 6, 0), "Latent Overshoots");
					DrawStr(context, TablePos(context, 6, 1), "%i%%", static_cast<uint32_t>((report->m_LatentOvershoots / static_cast<float>(report->m_FilteredMessages)) * 100));
				}

				{
					const char* postfix = nullptr;
					uint64_t pretty_data = PrettySize((report->m_ExportedMessages + report->m_LatentOvershoots) * sizeof(SDiagnosticMessageData), &postfix);

					DrawStr(context, TablePos(context, 7, 0), "Transferred Data");
					DrawStr(context, TablePos(context, 7, 1), "%i%s", pretty_data, postfix);
				}
			}

			// "Current messages"
			if (!report->m_Steps.empty())
			{
				/*  { n, 2 }
					Validation Error  /s
					...

					Latent Undershoots #%
					Latent Overshoots  #%
					Transferred Data   #
					...
				*/

				const SReportStep& step = report->m_Steps.back();

				uint64_t count = 0;

				uint32_t type_count = 0;
				for (uint32_t i = 0; i < ARRAY_LENGTH(step.m_ErrorCounts); i++)
				{
					type_count += step.m_ErrorCounts[i] > 0;
					count += step.m_ErrorCounts[i];
				}

				float widths[] = { 100, 25 };
				ConfigureTable(context, AosVector2{ 0, overview_extents.getY() + 10 }, 300, widths, 5 + type_count);

				DrawStr(context, TablePos(context, 0, 0), "Validation Error");
				DrawStr(context, TablePos(context, 0, 1), "/s");

				{
					DrawStr(context, TablePos(context, 2 + type_count, 0), "Latent Undershoots");
					DrawStr(context, TablePos(context, 2 + type_count, 1), "%i%%", static_cast<uint32_t>((step.m_LatentUndershoots / static_cast<float>(count)) * 100));
				}

				{
					DrawStr(context, TablePos(context, 3 + type_count, 0), "Latent Overshoots");
					DrawStr(context, TablePos(context, 3 + type_count, 1), "%i%%", static_cast<uint32_t>((step.m_LatentOvershoots / static_cast<float>(count)) * 100));
				}

				{
					const char* postfix = nullptr;
					uint64_t pretty_data = PrettySize(static_cast<uint64_t>(((count + step.m_LatentOvershoots) * sizeof(SDiagnosticMessageData)) / report->m_StepInterval), &postfix);

					DrawStr(context, TablePos(context, 4 + type_count, 0), "Transferred Data");
					DrawStr(context, TablePos(context, 4 + type_count, 1), "%i%s", pretty_data, postfix);
				}

				for (uint32_t i = 0, row = 1; i < ARRAY_LENGTH(step.m_ErrorCounts); i++)
				{
					if (step.m_ErrorCounts[i] == 0)
						continue;

					const char* type = "<null>";
					switch (i)
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
                        case VK_GPU_VALIDATION_ERROR_TYPE_SUBRESOURCE_UNINITIALIZED:
                            type = "SUBRESOURCE_UNINITIALIZED";
                            break;
					}

					DrawStr(context, TablePos(context, row, 0), "%s", type);
					

					const char* postfix = nullptr;
					uint64_t pretty_message_count = PrettyCount(static_cast<uint64_t>(step.m_ErrorCounts[i] / report->m_StepInterval), &postfix);
					
					DrawStr(context, TablePos(context, row, 1), "%lu%s", pretty_message_count, postfix);

					row++;
				}
			}
		}
	}

	return VK_SUCCESS;
}
