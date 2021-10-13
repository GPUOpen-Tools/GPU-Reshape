#pragma once

#include "Common.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

// Represents the snapshot of the shader cache
struct ShaderCacheData
{
	// Represents a single shader cache entry
	struct CacheEntry
	{
		std::vector<uint8_t>	 m_Blob;       ///< The modified blob
		VkShaderModuleCreateInfo m_CreateInfo; ///< The modified creation structure
	};

	std::unordered_map<uint64_t, CacheEntry> m_Entries; ///< All entries
};

class ShaderCache
{
public:
	/**
	 * Initialize the cache
	 * @param[in] device the vulkan device
	 */
	void Initialize(VkDevice device);

	/**
	 * Release this cache
	 */
	void Release();

	/**
	 * Enable auto serialization
	 * @param[in] path the path to the cache
	 * @param[in] threshold the cache miss count threshold at which it's serialized
	 * @param[in] growth_factor the growth factor applied to the threshold once auto serialized
	 */
	void SetAutoSerialization(const char* path, uint32_t threshold, float growth_factor);

	/**
	 * Invoke the asynchronous auto serialization
	 */
	void AutoSerialize();

	/**
	 * Deserialize this cache 
	 * @param[in] path the path to the cache
	 */
	void Deserialize(const char* path);

	/**
	 * Serialize this cache
	 * @param[in] path the path to the cache
	 */
	void Serialize(const char* path);

	/**
	 * Insert a new entry
	 * @param[in] feature_version_uid the uid of the feature set used
	 * @param[in] source the source shader creation info
	 * @param[in] recompiled the recompiled shader module against the feature set uid
	 */
	void Insert(uint64_t feature_version_uid, const VkShaderModuleCreateInfo& source, const VkShaderModuleCreateInfo& recompiled);

	/**
	 * Query for an entry
	 * @param[in] feature_version_uid the uid of the feature set used
	 * @param[in] create_info the source shader creation info
	 * @param[out] out the compiled cache hit
	 */
	bool Query(uint64_t feature_version_uid, const VkShaderModuleCreateInfo& create_info, VkShaderModuleCreateInfo* out);

	/**
	 * Get all pending entries for serialization
	 */
	uint32_t GetPendingEntries() const
	{
		return m_PendingShaderCacheEntries;
	}

private:
	/**
	 * Hash the creation info structure
	 * @param[in] create_info the structure to be hashed
	 */
	uint64_t HashCreateInfo(const VkShaderModuleCreateInfo& create_info);

	/**
	 * Internal serialization callback
	 * @param[in] data the data snapshot to be serialized
	 * @param[in] path the path to serialize to
	 */
	void SerializeInternal(const ShaderCacheData& data, const char* path);

private:
	VkDevice		m_Device; ///< The vulkan device
	std::mutex		m_Lock;	  ///< Generic data lock
	ShaderCacheData m_Data;	  ///< Current data snapshot

private: /* Threading */
	void ThreadEntry_AutoSerialization();

	std::atomic_bool		m_ThreadQueued;  ///< Is a thread queued
	std::atomic_bool		m_ThreadExit;	 ///< Thread exit flag
	std::condition_variable m_ThreadVar;	 ///< Thread wake conditional
	std::mutex				m_ThreadVarLock; ///< Thread lock
	std::thread				m_Thread;		 /// Thread handle

private: /* Configurable */
	const char* m_AutoSerializePath				= nullptr; ///< Auto serialization path
	uint32_t	m_AutoSerializationThreshold	= 0;	   ///< The pending count threshold at which auto serialization is invoked
	uint32_t	m_PendingShaderCacheEntries		= 0;	   ///< The number of pending entries to be serialized
	float		m_AutoSerializationGrowthFactor = 1;	   ///< The growth factor upon auto serialization applied to the treshold
};