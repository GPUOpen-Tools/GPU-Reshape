#include <ShaderCache.h>
#include <StateTables.h>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <CRC.h>
#include <DiagnosticRegistry.h>
#include <StreamHelpers.h>

static constexpr uint64_t kShaderCacheMagic = "<shader-cache>"_crc64;

// Serialziation helper
struct ShaderCacheHeaderData
{
	size_t m_EntryCount;
};

// Serialization helper
struct ShaderCacheEntryData
{
	uint64_t m_Key;
	size_t m_BlobSize;
	int32_t m_Flags;
};

void ShaderCache::Initialize(VkDevice device)
{
	m_Device = device;

	// Start thread
	m_Thread = std::thread([](ShaderCache* cache) { cache->ThreadEntry_AutoSerialization(); }, this);
}

void ShaderCache::Release()
{
	{
		std::unique_lock<std::mutex> unique(m_ThreadVarLock);
		m_ThreadExit = true;
		m_ThreadVar.notify_all();
	}

	// Wait for all worker
	if (m_Thread.joinable())
	{
		m_Thread.join();
	}
}

void ShaderCache::SetAutoSerialization(const char * path, uint32_t threshold, float growth_factor)
{
	m_AutoSerializePath = path;
	m_AutoSerializationThreshold = threshold;
	m_AutoSerializationGrowthFactor = growth_factor;
}

void ShaderCache::AutoSerialize()
{
	std::lock_guard<std::mutex> guard(m_Lock);

	if (m_PendingShaderCacheEntries > 0)
	{
		DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
		if (!m_ThreadQueued && table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, "Signalled async cache serialization...");
		}

		// Notify the serialization thread
		m_ThreadQueued = true;
		m_ThreadVar.notify_one();

		// Arbitary growth factor
		m_AutoSerializationThreshold = static_cast<uint32_t>(m_AutoSerializationThreshold * m_AutoSerializationGrowthFactor);
		m_PendingShaderCacheEntries = 0;
	}
}

void ShaderCache::ThreadEntry_AutoSerialization()
{
	for (;;)
	{
		// Wait for incoming requests
		std::unique_lock<std::mutex> unique(m_ThreadVarLock);
		m_ThreadVar.wait(unique, [&] { 
			if (m_ThreadExit) return true;

			bool state = true;
			return m_ThreadQueued.compare_exchange_strong(state, false);
		});

		if (m_ThreadExit)
		{
			return;
		}

		// Create a copy of the shader cache
		ShaderCacheData data;
		{
			std::lock_guard<std::mutex> guard(m_Lock);
			data = m_Data;
		}

		// Lock free serialization
		SerializeInternal(data, m_AutoSerializePath);
	}
}

void ShaderCache::Deserialize(const char* path)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(m_Device));

	// Attempt to open cache file
	std::ifstream stream(path, std::ios::binary);
	if (!stream.good())
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Failed to open cache file for reading");
		}
		return;
	}

	uint64_t magic;
	Read(stream, magic);

	if (magic != kShaderCacheMagic)
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "Shader cache corrupted, discarding");
		}
		return;
	}

	uint64_t version;
	Read(stream, version);

	if (version != ComputeCRC64(AVA_VULKAN_LAYERS_VERSION))
    {
        if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
        {
            table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "Shader cache version mismatch, discarding");
        }
        return;
    }

	std::vector<uint8_t> staging_buffer;

	std::lock_guard<std::mutex> guard(m_Lock);

	ShaderCacheHeaderData header;
	Read(stream, header);

	for (size_t i = 0; i < header.m_EntryCount; i++)
	{
		ShaderCacheEntryData entry_data;
		Read(stream, entry_data);

		staging_buffer.resize(entry_data.m_BlobSize);
		stream.read(reinterpret_cast<char*>(staging_buffer.data()), entry_data.m_BlobSize);

		// Pre-existing entry takes priority
		if (m_Data.m_Entries.count(entry_data.m_Key))
			continue;

		ShaderCacheData::CacheEntry& entry = m_Data.m_Entries[entry_data.m_Key];
		entry.m_Blob.swap(staging_buffer);

		entry.m_CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		entry.m_CreateInfo.flags = static_cast<VkShaderModuleCreateFlags>(entry_data.m_Flags);
		entry.m_CreateInfo.pCode = reinterpret_cast<uint32_t*>(entry.m_Blob.data());
		entry.m_CreateInfo.codeSize = entry.m_Blob.size();
	}

	state->m_DiagnosticRegistry->GetLocationRegistry()->GetData()->Deserialize(stream);

	if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[512];
		FormatBuffer(buffer, "Deserialized cache from '%s' [%ib]", path, static_cast<int32_t>(stream.tellg()));
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	stream.close();

	if (stream.fail() && table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
	{
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Deserialization failed");
	}
}

void ShaderCache::Serialize(const char* path)
{
	std::lock_guard<std::mutex> guard(m_Lock);

	SerializeInternal(m_Data, path);

	m_PendingShaderCacheEntries = 0;
}

void ShaderCache::SerializeInternal(const ShaderCacheData& data, const char* path)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(m_Device));

	// Attempt to open cache file
	std::ofstream stream(path, std::ios::binary);
	if (!stream.good())
	{
		if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Failed to open cache file for writing");
		}
		return;
	}

	stream.seekp(sizeof(kShaderCacheMagic) + sizeof(uint64_t));

	ShaderCacheHeaderData header;
	header.m_EntryCount = data.m_Entries.size();
	Write(stream, header);

	for (auto&& kv : data.m_Entries)
	{
		ShaderCacheEntryData entry_data;
		entry_data.m_Key = kv.first;
		entry_data.m_BlobSize = kv.second.m_Blob.size();
		entry_data.m_Flags = static_cast<int32_t>(kv.second.m_CreateInfo.flags);
		Write(stream, entry_data);

		stream.write(reinterpret_cast<const char*>(kv.second.m_Blob.data()), kv.second.m_Blob.size());
	}

	if (table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		char buffer[512];
		FormatBuffer(buffer, "Serialized cache to '%s' [%ib]", path, static_cast<int32_t>(stream.tellp()));
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
	}

	// Copied for async serialization
	// Not very elegant, but not noticable
	ShaderLocationRegistryData location_data = state->m_DiagnosticRegistry->GetLocationRegistry()->CopyData();
	location_data.Serialize(stream);

	stream.seekp(0);
	Write(stream, kShaderCacheMagic);
	Write(stream, ComputeCRC64(AVA_VULKAN_LAYERS_VERSION));

	stream.close();

	if (stream.fail() && table->m_CreateInfoAVA.m_LogCallback && (table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_INFO))
	{
		table->m_CreateInfoAVA.m_LogCallback(table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, "Serialization failed");
	}
}

void ShaderCache::Insert(uint64_t feature_version_uid, const VkShaderModuleCreateInfo & source, const VkShaderModuleCreateInfo & recompiled)
{
	uint64_t hash = 0;
	CombineHash(hash, feature_version_uid);
	CombineHash(hash, HashCreateInfo(source));

	bool needs_serialization = false;
	{
		std::lock_guard<std::mutex> guard(m_Lock);

		ShaderCacheData::CacheEntry& entry = m_Data.m_Entries[hash];

		entry.m_Blob.resize(recompiled.codeSize);
		std::memcpy(entry.m_Blob.data(), recompiled.pCode, recompiled.codeSize);

		entry.m_CreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		entry.m_CreateInfo.flags = source.flags;
		entry.m_CreateInfo.pCode = reinterpret_cast<uint32_t*>(entry.m_Blob.data());
		entry.m_CreateInfo.codeSize = entry.m_Blob.size();

		needs_serialization = m_AutoSerializePath && (++m_PendingShaderCacheEntries >= m_AutoSerializationThreshold);
	}

	if (needs_serialization)
	{
		AutoSerialize();
	}
}

bool ShaderCache::Query(uint64_t feature_version_uid, const VkShaderModuleCreateInfo & create_info, VkShaderModuleCreateInfo * out)
{
	uint64_t hash = 0;
	CombineHash(hash, feature_version_uid);
	CombineHash(hash, HashCreateInfo(create_info));

	std::lock_guard<std::mutex> guard(m_Lock);

	auto it = m_Data.m_Entries.find(hash);
	if (it == m_Data.m_Entries.end())
		return false;

	*out = it->second.m_CreateInfo;
	return true;
}

uint64_t ShaderCache::HashCreateInfo(const VkShaderModuleCreateInfo & create_info)
{
	uint64_t hash = 0;

	CombineHash(hash, create_info.codeSize);
	CombineHash(hash, create_info.flags);

	auto data = reinterpret_cast<const uint8_t*>(create_info.pCode);
	CombineHash(hash, ComputeCRC64Buffer(data, data + create_info.codeSize));

	return hash;
}
