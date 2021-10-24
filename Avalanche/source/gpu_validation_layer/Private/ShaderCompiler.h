#pragma once

#include "Shader.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <list>
#include <condition_variable>

// Represents a single shader compilation job
struct ShaderJob
{
	HSourceShader*		 m_SourceShader;	   ///< The shader to instrument
	HInstrumentedShader* m_InstrumentedShader; ///< The output shader
	uint32_t			 m_Features = 0;       ///< The instrumentation feature set
};

// Completion functor
using FShaderCompilerCompletionFunctor = std::function<void(uint64_t head, VkResult result)>;

class ShaderCompiler
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
	 * Push a set of jobs
	 * @param[in] jobs the job descriptions
	 * @param[in] job_count the number of jobs within jobs
	 * @param[in] functor the callback to be invoked upon completion of all jobs, regardless of failure
	 */
	void Push(const ShaderJob* jobs, uint32_t job_count, const FShaderCompilerCompletionFunctor& functor);

	/**
	 * Get the commit index
	 * Represents the head revision of the shader compiler
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
		FShaderCompilerCompletionFunctor m_Functor;
		std::atomic<uint32_t>			 m_Pending;
		VkResult						 m_Result;
	};

	// Represents a single queued job
	struct QueuedJob
	{
		QueuedJobContext* m_Context;
		ShaderJob		  m_Job;
	};

	/**
	 * Ensure that the workers are ready for compilation
	 */
	void PrepareWorkers();

	/**
	 * Compile a given job
	 * @param[in] job the job to be compiled
	 */
	VkResult Compile(const ShaderJob& job);

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