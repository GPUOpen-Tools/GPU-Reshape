#include <ShaderCompiler.h>
#include <DiagnosticRegistry.h>
#include <DiagnosticAllocator.h>
#include <ShaderCache.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <cstring>

#include <spirv-tools/optimizer.hpp>
#include <SPIRV/Pass.h>

// Enable to dump all injected SPIRV to file
#ifndef SHADER_COMPILER_DUMP_SPIRV
#   define SHADER_COMPILER_DUMP_SPIRV 0
#endif

#if SHADER_COMPILER_DUMP_SPIRV
#   include <spirv-tools/libspirv.h>
#endif

template<size_t LENGTH>
static void ShaderCompilerFormatFeatureBuffer(char(&buffer)[LENGTH], uint32_t feature_set)
{
	buffer[0] = 0;

	// Basic instrumentation
	uint32_t basic_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_BASIC;
	if (basic_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_BASIC)
	{
		FormatBuffer(buffer, "%sINSTRUMENTATION_SET_BASIC ", buffer);
	}
	else
	{
		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS)
			FormatBuffer(buffer, "%sSHADER_RESOURCE_ADDRESS_BOUNDS ", buffer);

		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY)
			FormatBuffer(buffer, "%sSHADER_EXPORT_STABILITY ", buffer);

		if (basic_mask & VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS)
			FormatBuffer(buffer, "%sSHADER_RUNTIME_ARRAY_BOUNDS ", buffer);
	}

	// Concurrency instrumentation
	uint32_t concurrency_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_CONCURRENCY;
	if (concurrency_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_CONCURRENCY)
	{
		FormatBuffer(buffer, "%sINSTRUMENTATION_SET_CONCURRENCY ", buffer);
	}
	else
	{
		if (concurrency_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE)
			FormatBuffer(buffer, "%sSHADER_RESOURCE_DATA_RACE ", buffer);
	}

	// Data residency instrumentation
	uint32_t dataresidency_mask = feature_set & VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_DATA_RESIDENCY;
	if (dataresidency_mask == VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_DATA_RESIDENCY)
	{
		FormatBuffer(buffer, "%sINSTRUMENTATION_SET_DATA_RESIDENCY", buffer);
	}
	else
	{
		if (dataresidency_mask & VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION)
			FormatBuffer(buffer, "%sSHADER_RESOURCE_INITIALIZATION", buffer);
	}
}

void ShaderCompiler::Initialize(VkDevice device, uint32_t worker_count)
{
	m_Device = device;
	m_RequestedWorkerCount = worker_count;
}

void ShaderCompiler::Release()
{
	// Post quit
	{
		std::lock_guard<std::mutex> guard(m_ThreadVarLock);
		m_ThreadExit = true;
		m_ThreadVar.notify_all();
	}

	// Diagnostic
	if (!m_Workers.empty())
	{
		DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
		{
			char buffer[512];
			FormatBuffer(buffer, "Stopping %i shader compiler workers...", m_RequestedWorkerCount);
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
		}
	}

	// Wait for all workers
	for (std::thread& worker : m_Workers)
	{
		if (!worker.joinable())
			continue;

		worker.join();
	}
}

void ShaderCompiler::Push(const ShaderJob* jobs, uint32_t job_count, const FShaderCompilerCompletionFunctor & functor)
{
	PrepareWorkers();

	// Push commit index
	m_CommitIndex += job_count;

	// Prepare shared context
	auto context = new QueuedJobContext();
	context->m_Pending = job_count;
	context->m_Functor = functor;
	context->m_Result = VK_SUCCESS;

	QueuedJob queued;
	queued.m_Context = context;

	// Push each job individually, no need to batch them
	std::lock_guard<std::mutex> guard(m_ThreadVarLock);
	for (uint32_t i = 0; i < job_count; i++)
	{
		queued.m_Job = jobs[i];
		m_QueuedJobs.push(std::move(queued));
		m_ThreadVar.notify_all();
	}
}

void ShaderCompiler::PrepareWorkers()
{
	if (!m_Workers.empty())
		return;

	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[512];
		FormatBuffer(buffer, "Starting %i shader compiler workers...", m_RequestedWorkerCount);
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	// Start workers
	for (uint32_t i = 0; i < m_RequestedWorkerCount; i++)
	{
		m_Workers.emplace_back([](ShaderCompiler* compiler) { compiler->ThreadEntry_Compiler(); }, this);
	}
}

VkResult ShaderCompiler::Compile(const ShaderJob& job)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(m_Device));

	// Get version uid for caching
	uint64_t feature_version_uid = state->m_DiagnosticRegistry->GetFeatureVersionUID(job.m_Features);

	// Diagnostic feature str
	char feature_buffer[256];
	ShaderCompilerFormatFeatureBuffer(feature_buffer, job.m_Features);

	// For sanity
	job.m_InstrumentedShader->m_CreateInfo = {};

	// Attempt to query cache first
	if (!state->m_ShaderCache->Query(feature_version_uid, job.m_SourceShader->m_CreateInfo, &job.m_InstrumentedShader->m_CreateInfo))
	{
		// Prepare state
		SPIRV::ShaderState shader_state;
		shader_state.m_DebugName = job.m_SourceShader->m_Name.c_str();
		shader_state.m_DeviceState = state;
		shader_state.m_DeviceDispatchTable = table;

		// Initialize optimizer
		spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1);

		// Default message handler, proxies through user logger
		optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message)
		{
			if (table->m_CreateInfoAVA.m_LogCallback)
			{
				// Translate severity
				VkGPUValidationLogSeverity severity;
				switch (level)
				{
				default:
					severity = VK_GPU_VALIDATION_LOG_SEVERITY_ERROR;
					break;
				case SPV_MSG_WARNING:
					severity = VK_GPU_VALIDATION_LOG_SEVERITY_WARNING;
					break;
				case SPV_MSG_INFO:
				case SPV_MSG_DEBUG:
					severity = VK_GPU_VALIDATION_LOG_SEVERITY_INFO;
					break;
				}

				if (!(table->m_CreateInfoAVA.m_LogSeverityMask & severity))
					return;

				char buffer[1024];
				FormatBuffer(buffer, "[SPIRV] Optimization Error\n\t%i:%i [%i] - %s", (int32_t)position.line, (int32_t)position.column, (int32_t)position.index, message);

				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, severity, __FILE__, __LINE__, buffer);
			}
		});

		// Register allocator
		state->m_DiagnosticAllocator->Register(&shader_state, &optimizer);

		// Register all passes to given optimizer
		state->m_DiagnosticRegistry->Register(job.m_Features, &shader_state, &optimizer);

		// Uncomment to debug IL issues (slow as hell)
		// optimizer.SetPrintAll(&std::cout);

		// May fail due to unsupported instruction sets
		if (optimizer.Run(job.m_SourceShader->m_CreateInfo.pCode, job.m_SourceShader->m_CreateInfo.codeSize / sizeof(uint32_t), &job.m_InstrumentedShader->m_SpirvCache, spvtools::ValidatorOptions(), true))
		{
			job.m_InstrumentedShader->m_CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
			job.m_InstrumentedShader->m_CreateInfo.flags = job.m_SourceShader->m_CreateInfo.flags;
			job.m_InstrumentedShader->m_CreateInfo.pCode = job.m_InstrumentedShader->m_SpirvCache.data();
			job.m_InstrumentedShader->m_CreateInfo.codeSize = job.m_InstrumentedShader->m_SpirvCache.size() * sizeof(uint32_t);

			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
			{
				char buffer[512];
				FormatBuffer(buffer, "[SPIRV] Recompiled shader '%s' { %s}", job.m_SourceShader->m_Name.c_str(), feature_buffer);
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
			}
		}
		else
		{
			if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
			{
				char buffer[512];
				FormatBuffer(buffer, "[SPIRV] JIT recompilation failed to shader '%s' { %s}", job.m_SourceShader->m_Name.c_str(), feature_buffer);
				table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, buffer);
			}
		}
	}
	else
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
		{
			char buffer[512];
			FormatBuffer(buffer, "[SPIRV] Cache hit shader '%s' { %s}", job.m_SourceShader->m_Name.c_str(), feature_buffer);
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
		}
	}

	// Debugging
#if SHADER_COMPILER_DUMP_SPIRV
    {
	    // Write original module
        std::ofstream original_stream("spirv/" + job.m_SourceShader->m_Name + "_Original.txt");
        if (original_stream.good())
        {
            spv_text text;
            if (spvBinaryToText(
                    state->m_Context,
                    job.m_SourceShader->m_CreateInfo.pCode, job.m_SourceShader->m_CreateInfo.codeSize / sizeof(uint32_t),
                    SPV_BINARY_TO_TEXT_OPTION_INDENT,
                    &text,
                    nullptr
            ) == SPV_SUCCESS)
            {
                original_stream.write(text->str, text->length);
            }
        }

        // Write the injected module
        std::ofstream injected_stream("spirv/" + job.m_SourceShader->m_Name + "_Injected.txt");
        if (injected_stream.good())
        {
            spv_text text;
            if (spvBinaryToText(
                    state->m_Context,
                    job.m_InstrumentedShader->m_CreateInfo.pCode, job.m_InstrumentedShader->m_CreateInfo.codeSize / sizeof(uint32_t),
                    SPV_BINARY_TO_TEXT_OPTION_INDENT,
                    &text,
                    nullptr
            ) == SPV_SUCCESS)
            {
                injected_stream.write(text->str, text->length);
            }
        }
	}
#endif

	// Pass down call chain
	VkResult result = table->m_CreateShaderModule(m_Device, &job.m_InstrumentedShader->m_CreateInfo, nullptr, &job.m_InstrumentedShader->m_Module);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Insert to cache if needed
	if (job.m_SourceShader->m_CreateInfo.pCode != job.m_InstrumentedShader->m_CreateInfo.pCode)
	{
		state->m_ShaderCache->Insert(feature_version_uid, job.m_SourceShader->m_CreateInfo, job.m_InstrumentedShader->m_CreateInfo);
	}

	return VK_SUCCESS;
}

void ShaderCompiler::ThreadEntry_Compiler()
{
	for (;;)
	{
		QueuedJob queued;

		// Wait for incoming jobs
		{
			std::unique_lock<std::mutex> unique(m_ThreadVarLock);
			m_ThreadVar.wait(unique, [&] {
				if (m_ThreadExit) return true;

				if (!m_QueuedJobs.empty()) {
					queued = m_QueuedJobs.front();
					m_QueuedJobs.pop();
					return true;
				}
				return false;
			});
		}

		// May be exit
		if (m_ThreadExit)
		{
			return;
		}

		// Attempt to compile the job
		VkResult result = Compile(queued.m_Job);
		if (result != VK_SUCCESS)
		{
			queued.m_Context->m_Result = result;
		}

		// Get predicted head
		uint64_t head = m_CompleteCounter + 1;

		// Last job of context?
		if (--queued.m_Context->m_Pending == 0)
		{
			{ std::lock_guard<std::mutex> step_lock(m_JobCompletionStepLock); }

			// Invoke response
			queued.m_Context->m_Functor(head, queued.m_Context->m_Result);
			delete queued.m_Context;
		}

		// Increment head
		// Must be done as a separate operation
		++m_CompleteCounter;
	}
}
