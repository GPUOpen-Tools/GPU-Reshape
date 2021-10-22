#include <ShaderLocationRegistry.h>
#include <algorithm>
#include <cstring>
#include <CRC.h>
#include <StreamHelpers.h>

static void StripFolders(std::string& path)
{
	size_t index = path.find_last_of("\\/");
	if (index == std::string::npos)
		return;

	path = path.substr(index + 1);
}

static void CleanPath(std::string& path)
{
    // Remove all quotations deliminators
    size_t index;
    while ((index = path.rfind('\"')) != std::string::npos)
    {
        path.erase(path.begin() + index);
    }
}

static void CleanDXCPath(std::string& path)
{
    // Remove all double path deliminators
    size_t index;
    while ((index = path.rfind("\\\\")) != std::string::npos)
    {
        path.erase(path.begin() + index);
    }

    // Remove quotations
    while ((index = path.rfind('\"')) != std::string::npos)
    {
        path.erase(path.begin() + index);
    }
}

static uint64_t GetExtractHash(uint16_t fileUID, const char* function_name, uint32_t line, uint32_t column)
{
	uint64_t hash = 0;
	CombineHash(hash, fileUID);
	CombineHash(hash, ComputeCRC64(function_name ? function_name : ""));
	CombineHash(hash, line);
	CombineHash(hash, column);
	return hash;
}

void ShaderLocationRegistry::Initialize(const VkGPUValidationCreateInfoAVA & create_info)
{
	m_CreateInfo = create_info;
}

const std::vector<ShaderLocationMapping>& ShaderLocationRegistry::RegisterDXCSourceExtract(const char* module_name, const char* module_path, const char* source)
{
	std::lock_guard<std::mutex> guard(m_Lock);

	// Cache length
	size_t source_length = std::strlen(source);

    // Compute source hash
	uint64_t hash = ComputeCRC64Buffer(reinterpret_cast<const uint8_t*>(source), reinterpret_cast<const uint8_t*>(source + source_length));

	// Module may already have been reflected
	auto extract_it = m_Data.m_SourceExtracts.find(module_name);
	if (extract_it != m_Data.m_SourceExtracts.end())
    {
	    // Compare hash against all
	    for (const ShaderLocationRegistryData::DXCSourceExtract& extract : (*extract_it).second)
        {
	        if (extract.m_SourceHash == hash)
            {
	            return extract.m_Mappings;
            }
        }
    }

	// Append new extract
    m_Data.m_SourceExtracts[module_name].emplace_back();

	// Prepare extract
    ShaderLocationRegistryData::DXCSourceExtract& extract = m_Data.m_SourceExtracts[module_name].back();
    extract.m_SourceHash = hash;

	// Current file
	ShaderLocationRegistryData::File* file = nullptr;

	// Hold a local mapping of source files
	// Must not create conflicts!
	std::unordered_map<std::string, ShaderLocationRegistryData::File*> local_mapping;

	// Previous preprocessed begin
	size_t preprocessed_begin = 0;

	// Scan for all line endings
	for (size_t i = 0; i < source_length; i++)
	{
		switch (source[i])
		{
			default:
				break;
			case '#': 
			{
				uint32_t line = UINT32_MAX;
				char file_path[256]{};

				if (sscanf(&source[i], "#line %i \"%s\"", &line, file_path) == 2)
				{
                    // Patch source of previous file
                    // Always done even if a continuation
                    if (file)
                    {
                        file->m_Source += std::string(source, preprocessed_begin, i - preprocessed_begin);
                    }

                    // Record beginning
                    preprocessed_begin = i;

                    /* At this one one of two things may happen:
                     *  1. A new virtual file
                     *  2. Continuation of a previous file at a potentially different offset
                     * */

				    // Continuation of a previous file?
				    auto local_mapping_it = local_mapping.find(file_path);
				    if (local_mapping_it != local_mapping.end())
                    {
				        file = (*local_mapping_it).second;

				        // Expand line count
				        // None of the dummy range should be accessed
				        if (line > 0)
                        {
				            file->m_PreprocessedLineOffsets.resize(line - 1);
                        }

				        break;
                    }

                    // Allocate uid
                    auto uid = static_cast<uint16_t>(m_Data.m_Files.size());

                    // Insert new file
                    file = (m_Data.m_Files[uid] = new ShaderLocationRegistryData::File());
                    file->m_Module = module_name;
                    file->m_ModulePath = module_path;
                    file->m_Path = file_path;
                    file->m_Source = "";

                    // Insert local mapping
                    local_mapping[file_path] = file;

                    // Remove DXC specific endings
                    CleanPath(file->m_ModulePath);
                    CleanPath(file->m_Path);

                    // May strip if requested
                    if (m_CreateInfo.m_StripFolders)
                    {
                        StripFolders(file->m_ModulePath);
                        StripFolders(file->m_Path);
                    }

                    // Prepare mapping path
                    std::string mapping_path = file_path;
                    {
                        // The InSource paths are not consistent with the OpLine file operands
                        // ( Which is really weird, but the blame lies on Microsoft )
                        CleanDXCPath(mapping_path);
                    }

                    // Prepare mapping
                    extract.m_Mappings.push_back({ mapping_path, uid });
				}
				break;
			}
			case '\n':
			{
				ShaderLocationRegistryData::File::Line line{};
				line.m_Offset = static_cast<uint32_t>(file->m_Source.size() + (i - preprocessed_begin));
				file->m_PreprocessedLineOffsets.push_back(line);
				break;
			}
		}
	}

    // Patch source of last file
    if (file)
    {
        file->m_Source += std::string(source, preprocessed_begin, std::strlen(source) - preprocessed_begin);
    }

	return extract.m_Mappings;
}

uint32_t ShaderLocationRegistry::RegisterLineExtract(uint16_t fileUID, const char* function_name, uint32_t line, uint32_t column)
{
	std::lock_guard<std::mutex> guard(m_Lock);

	// Check against cache
	uint64_t cache_hash = GetExtractHash(fileUID, function_name, line, column);
	auto cache_it = m_Data.m_ExtractLUT.find(cache_hash);
	if (cache_it != m_Data.m_ExtractLUT.end())
	{
		return cache_it->second;
	}

	// Just assume the next uid
	auto uid = static_cast<uint32_t>(m_Data.m_Extracts.size());
	ShaderLocationRegistryData::File* file = m_Data.m_Files.at(fileUID);

	// Exceeded limit?
	if (uid >= (1 << kShaderLocationGUIDBits))
    {
        if (m_CreateInfo.m_LogCallback && (m_CreateInfo.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
        {
            m_CreateInfo.m_LogCallback(m_CreateInfo.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Shader location registry is out of extract space, consider increasing kShaderLocationGUIDBits");
        }
	    return UINT32_MAX;
    }

	// Create extract
	ShaderLocationRegistryData::Extract*& extract = m_Data.m_Extracts[uid];
	extract = new ShaderLocationRegistryData::Extract();
	extract->m_File = fileUID;

	// Safeguard
	uint32_t line_offset = std::min(static_cast<uint32_t>(file->m_PreprocessedLineOffsets.size() - 2), line - 1);

	// DXCs line operands are weird as hell, they can "leak" into the next line
	ShaderLocationRegistryData::File::Line preprocessed_line = file->m_PreprocessedLineOffsets.at(line_offset);
	if (line_offset < file->m_PreprocessedLineOffsets.size() - 1)
	{
		ShaderLocationRegistryData::File::Line next = file->m_PreprocessedLineOffsets.at(line_offset + 1);
		
		uint32_t line_characters = next.m_Offset - preprocessed_line.m_Offset;
		if (line_characters <= column)
		{
			column -= line_characters;
			line++;
			line_offset++;

			preprocessed_line = file->m_PreprocessedLineOffsets.at(line_offset);
		}
	}

	// Deduce extract positions
	uint32_t begin = preprocessed_line.m_Offset;
	uint32_t end = (line_offset == file->m_PreprocessedLineOffsets.size() - 1) ? (uint32_t)file->m_Source.length() : file->m_PreprocessedLineOffsets.at(line_offset + 1).m_Offset;

	// Filter out line endings and trim whitespaces
	extract->m_Extract = file->m_Source.substr(begin, end - begin);
	extract->m_Extract.erase(std::remove(extract->m_Extract.begin(), extract->m_Extract.end(), '\n'), extract->m_Extract.end());
	if (std::count(extract->m_Extract.begin(), extract->m_Extract.end(), ' ') && extract->m_Extract.find_first_not_of(' ') != std::string::npos)
	{
		extract->m_Extract = extract->m_Extract.substr(extract->m_Extract.find_first_not_of(' '), (extract->m_Extract.find_last_not_of(' ') - extract->m_Extract.find_first_not_of(' ')) + 1);;
	}

	extract->m_AVA.m_Extract = extract->m_Extract.c_str();

	if (function_name)
	{
		extract->m_FunctionName = function_name;
		extract->m_AVA.m_Function = extract->m_FunctionName.c_str();
	}
	else
	{
		extract->m_AVA.m_Function = nullptr;
	}

	extract->m_AVA.m_Module = file->m_Module.c_str();
    extract->m_AVA.m_ModuleFile = file->m_ModulePath.c_str();

	// Record span
	extract->m_AVA.m_File = file->m_Path.c_str();
	extract->m_AVA.m_Span.m_Begin.m_Offset = begin;
	extract->m_AVA.m_Span.m_Begin.m_Character = 0;
	extract->m_AVA.m_Span.m_Begin.m_Line = line;
	extract->m_AVA.m_Span.m_End.m_Offset = end;
	extract->m_AVA.m_Span.m_End.m_Character = end - begin;
	extract->m_AVA.m_Span.m_End.m_Line = line;

	m_Data.m_ExtractLUT[cache_hash] = uid;
	return uid;
}

uint32_t ShaderLocationRegistry::RegisterFileExtract(uint16_t fileUID, const char* function_name)
{
	std::lock_guard<std::mutex> guard(m_Lock);

	// Check against cache
	uint64_t cache_hash = GetExtractHash(fileUID, function_name, 0, 0);
	auto cache_it = m_Data.m_ExtractLUT.find(cache_hash);
	if (cache_it != m_Data.m_ExtractLUT.end())
	{
		return cache_it->second;
	}

	// Just assume the next uid
	auto uid = static_cast<uint32_t>(m_Data.m_Extracts.size());
	ShaderLocationRegistryData::File* file = m_Data.m_Files.at(fileUID);

    // Exceeded limit?
    if (uid >= (1 << kShaderLocationGUIDBits))
    {
        if (m_CreateInfo.m_LogCallback && (m_CreateInfo.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
        {
            m_CreateInfo.m_LogCallback(m_CreateInfo.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "Shader location registry is out of extract space, consider increasing kShaderLocationGUIDBits");
        }
        return UINT32_MAX;
    }

	// Create the extract
	ShaderLocationRegistryData::Extract*& extract = m_Data.m_Extracts[uid];
	extract = new ShaderLocationRegistryData::Extract();
	extract->m_File = fileUID;
	extract->m_AVA.m_Extract = nullptr;
	extract->m_AVA.m_Module = file->m_Module.c_str();
    extract->m_AVA.m_ModuleFile = file->m_ModulePath.c_str();
	extract->m_AVA.m_File = file->m_Path.c_str();
	extract->m_AVA.m_Span.m_Begin.m_Offset = UINT32_MAX;
	extract->m_AVA.m_Span.m_End.m_Offset = UINT32_MAX;

	if (function_name)
	{
		extract->m_FunctionName = function_name;
		extract->m_AVA.m_Function = extract->m_FunctionName.c_str();
	}
	else
	{
		extract->m_AVA.m_Function = nullptr;
	}

	m_Data.m_ExtractLUT[cache_hash] = uid;
	return uid;
}

bool ShaderLocationRegistry::GetExtractFromUID(uint32_t uid, VkGPUValidationSourceExtractAVA* out)
{
	std::lock_guard<std::mutex> guard(m_Lock);

	auto it = m_Data.m_Extracts.find(uid);
	if (it == m_Data.m_Extracts.end())
	    return false;

	*out = it->second->m_AVA;
	return true;
}

void ShaderLocationRegistryData::Serialize(std::ofstream& stream)
{
    Write(stream, m_SourceExtracts.size());

    for (auto&& kv : m_SourceExtracts)
    {
        Write(stream, kv.first.size());
        stream.write(kv.first.data(), kv.first.size());

        Write(stream, kv.second.size());
        for (const DXCSourceExtract& extract : kv.second)
        {
            Write(stream, extract.m_SourceHash);

            Write(stream, extract.m_Mappings.size());

            for (const ShaderLocationMapping& mapping : extract.m_Mappings)
            {
                Write(stream, mapping.m_Path.size());
                stream.write(mapping.m_Path.data(), mapping.m_Path.size());

                Write(stream, mapping.m_UID);
            }
        }
    }

	Write(stream, m_Files.size());

	for (auto&& kv : m_Files)
	{
		Write(stream, kv.first);

		Write(stream, kv.second->m_Module.size());
		stream.write(kv.second->m_Module.data(), kv.second->m_Module.size());

        Write(stream, kv.second->m_ModulePath.size());
        stream.write(kv.second->m_ModulePath.data(), kv.second->m_ModulePath.size());

        Write(stream, kv.second->m_Path.size());
        stream.write(kv.second->m_Path.data(), kv.second->m_Path.size());

		Write(stream, kv.second->m_Source.size());
		stream.write(kv.second->m_Source.data(), kv.second->m_Source.size());

		Write(stream, kv.second->m_PreprocessedLineOffsets.size());
		stream.write(reinterpret_cast<const char*>(kv.second->m_PreprocessedLineOffsets.data()), kv.second->m_PreprocessedLineOffsets.size() * sizeof(File::Line));
	}

	Write(stream, m_Extracts.size());

	for (auto&& kv : m_Extracts)
	{
		Write(stream, kv.first);

		Write(stream, kv.second->m_File);

		Write(stream, kv.second->m_FunctionName.size());
		stream.write(kv.second->m_FunctionName.data(), kv.second->m_FunctionName.size());

		Write(stream, kv.second->m_Extract.size());
		stream.write(kv.second->m_Extract.data(), kv.second->m_Extract.size());

        Write(stream, kv.second->m_BindingMappings.size());
		for (BindingMapping& mapping : kv.second->m_BindingMappings)
        {
            Write(stream, mapping.m_ID);
            Write(stream, mapping.m_Binding);
        }

		Write(stream, kv.second->m_AVA.m_Span);
	}
}

void ShaderLocationRegistryData::Deserialize(std::ifstream& stream)
{
    size_t source_extract_count;
    Read(stream, source_extract_count);

    for (uint64_t i = 0; i < source_extract_count; i++)
    {
        size_t module_size;
        Read(stream, module_size);

        std::string module;

        module.resize(module_size);
        stream.read(&module[0], module_size);

        size_t module_extract_count;
        Read(stream, module_extract_count);

        for (uint64_t j = 0; j < module_extract_count; j++)
        {
            DXCSourceExtract source_extract;

            Read(stream, source_extract.m_SourceHash);

            size_t mapping_count;
            Read(stream, mapping_count);

            source_extract.m_Mappings.resize(mapping_count);
            for (uint64_t k = 0; k < mapping_count; k++)
            {
                ShaderLocationMapping& mapping = source_extract.m_Mappings[k];

                size_t path_size;
                Read(stream, path_size);

                mapping.m_Path.resize(path_size);
                stream.read(&mapping.m_Path[0], path_size);

                Read(stream, mapping.m_UID);
            }

            m_SourceExtracts[module].push_back(source_extract);
        }
    }

	size_t file_count;
	Read(stream, file_count);

	for (uint64_t i = 0; i < file_count; i++)
	{
		uint16_t key;
		Read(stream, key);

		File*& file = m_Files[key];
		file = new ShaderLocationRegistryData::File();

		size_t path_size;
		Read(stream, path_size);

		file->m_Module.resize(path_size);
		stream.read(&file->m_Module[0], path_size);

        Read(stream, path_size);

        file->m_ModulePath.resize(path_size);
        stream.read(&file->m_ModulePath[0], path_size);

        Read(stream, path_size);

        file->m_Path.resize(path_size);
        stream.read(&file->m_Path[0], path_size);

		size_t source_size;
		Read(stream, source_size);

		file->m_Source.resize(source_size);
		stream.read(&file->m_Source[0], source_size);

		size_t line_offsets_size;
		Read(stream, line_offsets_size);

		file->m_PreprocessedLineOffsets.resize(line_offsets_size);
		stream.read(reinterpret_cast<char*>(file->m_PreprocessedLineOffsets.data()), line_offsets_size * sizeof(File::Line));
	}

	size_t extract_count;
	Read(stream, extract_count);

	for (uint64_t i = 0; i < extract_count; i++)
	{
		uint32_t key;
		Read(stream, key);

		Extract*& extract = m_Extracts[key];
		extract = new ShaderLocationRegistryData::Extract();

		Read(stream, extract->m_File);

		File* file = m_Files[extract->m_File];

		size_t fn_size;
		Read(stream, fn_size);

		extract->m_FunctionName.resize(fn_size);
		stream.read(&extract->m_FunctionName[0], fn_size);

		size_t extract_size;
		Read(stream, extract_size);

		extract->m_Extract.resize(extract_size);
		stream.read(&extract->m_Extract[0], extract_size);

		size_t mapping_count;
		Read(stream, mapping_count);

		extract->m_BindingMappings.resize(mapping_count);
		for (uint64_t j = 0; j < mapping_count; j++)
        {
            Read(stream, extract->m_BindingMappings[j].m_ID);
            Read(stream, extract->m_BindingMappings[j].m_Binding);
        }

		Read(stream, extract->m_AVA.m_Span);

		extract->m_AVA.m_Function = extract->m_FunctionName.c_str();
		extract->m_AVA.m_Extract = extract->m_Extract.c_str();
        extract->m_AVA.m_Module = file->m_Module.c_str();
        extract->m_AVA.m_ModuleFile = file->m_ModulePath.c_str();

        extract->m_AVA.m_File = file->m_Path.c_str();
	}

	RepopulateCache();
}

void ShaderLocationRegistryData::RepopulateCache()
{
	m_ExtractLUT.clear();

	// Reconstruct extract LUT
	for (auto kv : m_Extracts)
	{
		m_ExtractLUT[GetExtractHash(
			kv.second->m_File, 
			kv.second->m_FunctionName.c_str(),
			kv.second->m_AVA.m_Span.m_Begin.m_Line, 
			kv.second->m_AVA.m_Span.m_Begin.m_Character)] = kv.first;
	}
}

ShaderLocationRegistryData* ShaderLocationRegistry::GetData()
{
	std::lock_guard<std::mutex> guard(m_Lock);
	return &m_Data;
}

ShaderLocationRegistryData ShaderLocationRegistry::CopyData()
{
	std::lock_guard<std::mutex> guard(m_Lock);
	return m_Data;
}

void ShaderLocationRegistry::RegisterExtractBinding(uint32_t extract_uid, uint32_t binding_id, const ShaderLocationBinding &binding)
{
    std::lock_guard<std::mutex> guard(m_Lock);

    // Push binding
    ShaderLocationRegistryData::Extract* extract = m_Data.m_Extracts.at(extract_uid);
    extract->m_BindingMappings.push_back({binding_id, binding});
}

bool ShaderLocationRegistry::GetBindingMapping(uint32_t extract_uid, uint32_t binding_id, ShaderLocationBinding* out)
{
    std::lock_guard<std::mutex> guard(m_Lock);

    // Get extract
    ShaderLocationRegistryData::Extract* extract = m_Data.m_Extracts.at(extract_uid);

    // Attempt to match any mappings
    for (const ShaderLocationRegistryData::BindingMapping& mapping : extract->m_BindingMappings)
    {
        if (mapping.m_ID == binding_id)
        {
            *out = mapping.m_Binding;
            return true;
        }
    }

    return false;
}

