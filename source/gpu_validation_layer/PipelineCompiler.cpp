#include <PipelineCompiler.h>
#include <DiagnosticRegistry.h>
#include <ShaderCache.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <cstring>

#if PIPELINE_COMPILER_DEBUG
#   include <spirv-tools/libspirv.h>
#endif

static constexpr uint32_t kChunkedSegmentationFactor = 3;

void PipelineCompiler::Initialize(VkDevice device, uint32_t worker_count)
{
	m_Device = device;
	m_RequestedWorkerCount = worker_count;
}

void PipelineCompiler::Release()
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
			FormatBuffer(buffer, "Stopping %i pipeline compiler workers...", m_RequestedWorkerCount);
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

void PipelineCompiler::Push(const GraphicsPipelineJob& job, const FPipelineCompilerCompletionFunctor & functor)
{
	PrepareWorkers();

	// Increment commit index
	m_CommitIndex += job.m_CreateInfos.size();

	// Get chunked size
#if PIPELINE_COMPILER_DEBUG
    const uint32_t chunked_size = 1;
#else
	const uint32_t chunked_size = std::max(1u, static_cast<uint32_t>(job.m_CreateInfos.size() / (m_Workers.size() * kChunkedSegmentationFactor)));
#endif

	// Prepare the context
	auto context = new QueuedJobContext();
	context->m_Pending = 0;
	context->m_Pipelines.resize(job.m_CreateInfos.size());
	context->m_Functor = functor;
	context->m_Result = VK_SUCCESS;

	// Push job batches
	std::lock_guard<std::mutex> guard(m_ThreadVarLock);
	for (int32_t remaining = static_cast<uint32_t>(job.m_CreateInfos.size()); remaining > 0; remaining -= chunked_size)
	{
		uint32_t offset = static_cast<uint32_t>(job.m_CreateInfos.size()) - remaining;

		QueuedJob queued;
		queued.m_Context = context;
		queued.m_Type = EPipelineType::eGraphics;
		queued.m_PipelineOffset = offset;
		queued.m_GraphicsJob.m_Cache = job.m_Cache;

		uint32_t workgroup_size = std::min<uint32_t>(remaining, chunked_size);

		queued.m_GraphicsJob.m_CreateInfos.resize(workgroup_size);
		std::memcpy(queued.m_GraphicsJob.m_CreateInfos.data(), &job.m_CreateInfos[offset], sizeof(VkGraphicsPipelineCreateInfo) * workgroup_size);

		m_QueuedJobs.push(std::move(queued));
		context->m_Pending++;
	}

	m_ThreadVar.notify_all();
}

void PipelineCompiler::Push(const ComputePipelineJob & job, const FPipelineCompilerCompletionFunctor & functor)
{
	PrepareWorkers();

	// Increment commit index
	m_CommitIndex += job.m_CreateInfos.size();

	// Get chunked size
#if PIPELINE_COMPILER_DEBUG
    const uint32_t chunked_size = 1;
#else
	const uint32_t chunked_size = std::max(1u, static_cast<uint32_t>(job.m_CreateInfos.size() / (m_Workers.size() * kChunkedSegmentationFactor)));
#endif

	// Prepare the context
	auto context = new QueuedJobContext();
	context->m_Pending = 0;
	context->m_Pipelines.resize(job.m_CreateInfos.size());
	context->m_Functor = functor;
	context->m_Result = VK_SUCCESS;

	// Push job batches
	std::lock_guard<std::mutex> guard(m_ThreadVarLock);
	for (int32_t remaining = static_cast<uint32_t>(job.m_CreateInfos.size()); remaining > 0; remaining -= chunked_size)
	{
		uint32_t offset = static_cast<uint32_t>(job.m_CreateInfos.size()) - remaining;

		QueuedJob queued;
		queued.m_Context = context;
		queued.m_Type = EPipelineType::eCompute;
		queued.m_PipelineOffset = offset;
		queued.m_ComputeJob.m_Cache = job.m_Cache;

		uint32_t workgroup_size = std::min<uint32_t>(remaining, chunked_size);

		queued.m_ComputeJob.m_CreateInfos.resize(workgroup_size);
		std::memcpy(queued.m_ComputeJob.m_CreateInfos.data(), &job.m_CreateInfos[offset], sizeof(VkComputePipelineCreateInfo) * workgroup_size);

		m_QueuedJobs.push(std::move(queued));
		context->m_Pending++;
	}

	m_ThreadVar.notify_all();
}

void PipelineCompiler::PrepareWorkers()
{
	if (!m_Workers.empty())
		return;

	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[512];
		FormatBuffer(buffer, "Starting %i pipeline compiler workers...", m_RequestedWorkerCount);
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	// Start workers
	for (uint32_t i = 0; i < m_RequestedWorkerCount; i++)
	{
		m_Workers.emplace_back([](PipelineCompiler* compiler) { compiler->ThreadEntry_Compiler(); }, this);
	}
}

#if PIPELINE_COMPILER_DEBUG
VkResult DebugDumpFrontPipeline(VkDevice device, uint32_t offset, const void* extension)
{
    DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

    auto info = FindStructureType<PipelineJobDebugSource>(reinterpret_cast<const VkComputePipelineCreateInfo*>(extension), VK_STRUCTURE_TYPE_INTERNAL_PIPELINE_JOB_DEBUG_SOURCE);
    if (info)
    {
        for (auto shader : info->m_SourcePipeline->m_ShaderModules)
        {
            // Write original module
            std::ofstream original_stream("spirv/" + shader->m_SourceShader.m_Name + "_Original.txt");
            if (original_stream.good())
            {
                spv_text text;
                if (spvBinaryToText(
                    state->m_Context,
                    shader->m_SourceShader.m_CreateInfo.pCode, shader->m_SourceShader.m_CreateInfo.codeSize / sizeof(uint32_t),
                    SPV_BINARY_TO_TEXT_OPTION_INDENT,
                    &text,
                    nullptr
                ) == SPV_SUCCESS)
                {
                    original_stream.write(text->str, text->length);
                }
            }

            // Write the injected module
            std::ofstream injected_stream("spirv/" + shader->m_SourceShader.m_Name + "_Injected.txt");
            if (injected_stream.good())
            {
                spv_text text;
                if (spvBinaryToText(
                    state->m_Context,
                    shader->m_InstrumentedShader.m_CreateInfo.pCode, shader->m_InstrumentedShader.m_CreateInfo.codeSize / sizeof(uint32_t),
                    SPV_BINARY_TO_TEXT_OPTION_INDENT,
                    &text,
                    nullptr
                ) == SPV_SUCCESS)
                {
                    injected_stream.write(text->str, text->length);
                }
            }
        }
    }

    return VK_SUCCESS;
}
#endif

VkResult PipelineCompiler::Compile(QueuedJobContext* context, uint32_t offset, const GraphicsPipelineJob& job)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));

	VkResult result = table->m_CreateGraphicsPipelines(table->m_Device, job.m_Cache, static_cast<uint32_t>(job.m_CreateInfos.size()), job.m_CreateInfos.data(), nullptr, &context->m_Pipelines[offset]);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

VkResult PipelineCompiler::Compile(QueuedJobContext* context, uint32_t offset, const ComputePipelineJob& job)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));

	VkResult result = table->m_CreateComputePipelines(table->m_Device, job.m_Cache, static_cast<uint32_t>(job.m_CreateInfos.size()), job.m_CreateInfos.data(), nullptr, &context->m_Pipelines[offset]);
	if (result != VK_SUCCESS)
	{
		return result;
	}

	return VK_SUCCESS;
}

void PipelineCompiler::ThreadEntry_Compiler()
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
		switch (queued.m_Type)
		{
			case EPipelineType::eGraphics:
			{
				VkResult result = Compile(queued.m_Context, queued.m_PipelineOffset, queued.m_GraphicsJob);
				if (result != VK_SUCCESS)
				{
					queued.m_Context->m_Result = result;
				}
				break;
			}
			case EPipelineType::eCompute:
			{
				VkResult result = Compile(queued.m_Context, queued.m_PipelineOffset, queued.m_ComputeJob);
				if (result != VK_SUCCESS)
				{
					queued.m_Context->m_Result = result;
				}
				break;
			}
		}

		// Get predicted head
		uint64_t head = 0;
		switch (queued.m_Type)
		{
			case EPipelineType::eGraphics:
			{
				head = (m_CompleteCounter + queued.m_GraphicsJob.m_CreateInfos.size());
				break;
			}
			case EPipelineType::eCompute:
			{
				head = (m_CompleteCounter + queued.m_ComputeJob.m_CreateInfos.size());
				break;
			}
		}

		// Last job of context?
		if (--queued.m_Context->m_Pending == 0)
		{
			{ std::lock_guard<std::mutex> step_lock(m_JobCompletionStepLock); }

			// Invoke response
			queued.m_Context->m_Functor(head, queued.m_Context->m_Result, queued.m_Context->m_Pipelines.data());
			delete queued.m_Context;
		}
		
		// Increment head
		// Must be done as a separate operation
		switch (queued.m_Type)
		{
			case EPipelineType::eGraphics:
			{
				m_CompleteCounter += queued.m_GraphicsJob.m_CreateInfos.size();
				break;
			}
			case EPipelineType::eCompute:
			{
				m_CompleteCounter += queued.m_ComputeJob.m_CreateInfos.size();
				break;
			}
		}
	}
}
