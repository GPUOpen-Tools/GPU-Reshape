#pragma once

#include "Pipeline.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <list>
#include <condition_variable>
#include <CRC.h>

#ifndef PIPELINE_COMPILER_DEBUG
#   define PIPELINE_COMPILER_DEBUG 0
#endif

#if PIPELINE_COMPILER_DEBUG
#   include <spirv-tools/libspirv.h>

// Internal structure type
static constexpr auto VK_STRUCTURE_TYPE_INTERNAL_PIPELINE_JOB_DEBUG_SOURCE = static_cast<VkStructureType>("pipeline_job_debug_source"_crc64);

struct PipelineJobDebugSource
{
    VkStructureType m_Type = VK_STRUCTURE_TYPE_INTERNAL_PIPELINE_JOB_DEBUG_SOURCE;
    const void*     m_Next;
    HPipeline*      m_SourcePipeline;
};
#endif

// Represents a graphics pipeline batch compilation job
struct GraphicsPipelineJob
{
	VkPipelineCache m_Cache;
	std::vector<VkGraphicsPipelineCreateInfo> m_CreateInfos;
};

// Represents a compute pipeline batch compilation job
struct ComputePipelineJob
{
	VkPipelineCache m_Cache;
	std::vector<VkComputePipelineCreateInfo> m_CreateInfos;
};

// Completion functor
using FPipelineCompilerCompletionFunctor = std::function<void(uint64_t head, VkResult result, VkPipeline* pipelines)>;

class PipelineCompiler
{
public:
	/**
	 * Initialize the compiler
	 * @param[in] device the vulkan device
	 * @param[in] worker_count the number of worker threads to (lazyily) spawn
	 */
	void Initialize(VkDevice device, uint32_t worker_count);

	/**
	 * Release this compiler
	 */
	void Release();

	/**
	 * Push a job
	 * @param[in] job the job to be pushed
	 * @param[in] functor the callback to be invoked upon completion of the job, regardless of failure
	 */
	void Push(const GraphicsPipelineJob& job, const FPipelineCompilerCompletionFunctor& functor);

	/**
	 * Push a job
	 * @param[in] job the job to be pushed
	 * @param[in] functor the callback to be invoked upon completion of the job, regardless of failure
	 */
	void Push(const ComputePipelineJob& job, const FPipelineCompilerCompletionFunctor& functor);

	/**
	 * Get the commit index
	 * Represents the head revision of the pipeline compiler
	 */
	uint64_t GetCommit() const
	{
		return m_CommitIndex.load();
	}

	/**
	 * Check if a commit has been pushed
	 * @param[in] commit the commit to be checked against
	 */
	bool IsCommitPushed(uint64_t commit)
	{
		return commit <= m_CompleteCounter.load();
	}

	/**
	 * Check if a commit has been pushed against an arbitary head
	 * @param[in] head the head to compare against
	 * @param[in] commit the commit to be checked against
	 */
	bool IsCommitPushed(uint64_t head, uint64_t commit)
	{
		return commit <= head;
	}

	/**
	 * Get the number of pending commits before a given commit
	 * @param[in] commit the commit to be compared against
	 */
	uint32_t GetPendingCommits(uint64_t commit)
	{
		uint64_t counter = m_CompleteCounter.load();
		if (counter > commit)
			return 0;

		return static_cast<uint32_t>(commit - counter);
	}

	/**
	 * Lock the completion callbacks, useful for aggregation of commits
	 */
	void LockCompletionStep()
	{
		m_JobCompletionStepLock.lock();
	}

	/**
	 * Unlock the completion callbacks
	 */
	void UnlockCompletionStep()
	{
		m_JobCompletionStepLock.unlock();
	}

private:
	// Represents a shared job context
	struct QueuedJobContext
	{
		FPipelineCompilerCompletionFunctor m_Functor;
		std::atomic<uint32_t>			 m_Pending;
		std::vector<VkPipeline>			 m_Pipelines;
		VkResult						 m_Result;
	};

	// Represents a single queued job
	struct QueuedJob
	{
		QueuedJobContext* m_Context;
		EPipelineType m_Type;
		uint32_t m_PipelineOffset;
		GraphicsPipelineJob m_GraphicsJob;
		ComputePipelineJob m_ComputeJob;
	};

	/**
	 * Ensure that the workers are ready for compilation
	 */
	void PrepareWorkers();

	/**
	 * Compile a given job
	 * @param[in] context the shared context
	 * @param[in] offset the destination pipeline offset
	 * @param[in] job the job to be compiled
	 */
	VkResult Compile(QueuedJobContext* context, uint32_t offset, const GraphicsPipelineJob& job);

	/**
	 * Compile a given job
	 * @param[in] context the shared context
	 * @param[in] offset the destination pipeline offset
	 * @param[in] job the job to be compiled
	 */
	VkResult Compile(QueuedJobContext* context, uint32_t offset, const ComputePipelineJob& job);

	/**
	 * The worker thread entry point
	 */
	void ThreadEntry_Compiler();

private:
	VkDevice				m_Device;				 ///< The vulkan device
													 
	uint32_t				m_RequestedWorkerCount;  ///< The number of requested workers by the user
	std::list<std::thread>  m_Workers;				 ///< Currently active workers
	std::atomic_bool		m_ThreadExit;			 ///< Shared worker exit condition
	std::condition_variable m_ThreadVar;			 ///< Shared worker wake condition
	std::mutex				m_ThreadVarLock;		 ///< Shared worker wake lock
	std::mutex				m_JobCompletionStepLock; ///< Shared lock for job completion
	std::queue<QueuedJob>	m_QueuedJobs;			 ///< Shared job queue

	std::atomic<uint64_t>	m_CommitIndex{ 0 };		 ///< Current commit index
	std::atomic<uint64_t>	m_CompleteCounter{ 0 };	 ///< Current push index
};
