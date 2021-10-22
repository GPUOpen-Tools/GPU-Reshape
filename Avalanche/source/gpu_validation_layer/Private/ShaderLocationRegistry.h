#pragma once

#include "Common.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <fstream>
#include <mutex>

static constexpr auto kShaderLocationGUIDBits = 17;

// Represents virtual file mappings
struct ShaderLocationMapping
{
    std::string m_Path;
    uint16_t    m_UID;
};

// Represents an extract location to descriptor binding mapping
struct ShaderLocationBinding
{
    uint32_t m_SetIndex;
    uint32_t m_BindingIndex;
};

// Represents the shader location registry data snapshot
struct ShaderLocationRegistryData
{
	/**
	 * Serialize this snapshot to a stream
	 * @param[in] stream the destination stream
	 */
	void Serialize(std::ofstream& stream);

	/**
	 * Deserialize this snapshot from a stream
	 * @param[in] stream the source stream
	 */
	void Deserialize(std::ifstream& stream);

	// Represents a DXC source extract
	struct DXCSourceExtract
    {
	    uint64_t                           m_SourceHash;
	    std::vector<ShaderLocationMapping> m_Mappings;
    };

	// Represents a reflected file
	struct File
	{
        std::string			  m_Module;
        std::string			  m_ModulePath;
		std::string           m_Path;
		std::string			  m_Source;

		struct Line
		{
			uint32_t m_Offset;
		};

		std::vector<Line> m_PreprocessedLineOffsets;
	};

	// Represents an identified source location to binding mapping
	struct BindingMapping
    {
	    uint32_t              m_ID;
        ShaderLocationBinding m_Binding;
    };

	// Represents a reflected extract
	struct Extract
	{
		uint16_t m_File;

		// Requires separate storage
		std::string m_Extract;
		std::string m_FunctionName;

		// Binding mappings
		std::vector<BindingMapping> m_BindingMappings;

		// Descriptor
		VkGPUValidationSourceExtractAVA m_AVA;
	};

	// Lookups
	std::map<std::string, std::vector<DXCSourceExtract>> m_SourceExtracts;
	std::map<uint16_t, File*>			                 m_Files;
	std::map<uint32_t, Extract*>		                 m_Extracts;
	std::unordered_map<uint64_t, uint32_t>               m_ExtractLUT;

private:
	/**
	 * Repopulate the extract cache
	 */
	void RepopulateCache();
};

class ShaderLocationRegistry
{
public:
	/**
	 * Initialize this registry
	 * @param[in] create_info the creation info
	 */
	void Initialize(const VkGPUValidationCreateInfoAVA& create_info);

	/**
	 * Register a new file for reflection
	 * @param[in] name the filename
	 * @param[in] source the source associated with the filename
	 */
	const std::vector<ShaderLocationMapping>& RegisterDXCSourceExtract(const char* module_name, const char* module_path, const char* source);

	/**
	 * Register a line source extract
	 * @param[in] file the registered file uid
	 * @param[in] function_name the optional function name
	 * @param[in] line the line at which this extract starts
	 * @param[in] column the column at which this extracts starts
	 */
	uint32_t RegisterLineExtract(uint16_t file, const char* function_name, uint32_t line, uint32_t column);

	/**
	 * Register a file source extract
	 * @param[in] file the registered file uid
	 * @param[in] function_name the optional function name
	 */
	uint32_t RegisterFileExtract(uint16_t file, const char* function_name);

	/**
	 * Register an extract to descriptor binding mapping
	 * @param extract_uid the extract identifier
	 * @param binding_id the unique identifier to be passed during querying
	 * @param binding the descriptor binding mapping
	 */
	void RegisterExtractBinding(uint32_t extract_uid, uint32_t binding_id, const ShaderLocationBinding& binding);

	/**
	 * Get an extract binding mapping with a given unique identifier
	 * @param extract_uid the extract identifier
	 * @param binding_id the unique binding identifier passed during RegisterExtractBinding
	 * @param[out] out the written binding information if found
	 * @return true if found
	 */
	bool GetBindingMapping(uint32_t extract_uid, uint32_t binding_id, ShaderLocationBinding* out);

	/**
	 * Get the compiled source extract
	 * @param[in] uid the uid of the source extract
	 */
	bool GetExtractFromUID(uint32_t uid, VkGPUValidationSourceExtractAVA* out);

	/**
	 * Get the internally hosted data
	 * ! Not thread safe
	 */
	ShaderLocationRegistryData* GetData();

	/**
	 * Create a snapshot of the internally hosted data
	 */
	ShaderLocationRegistryData CopyData();

private:
	std::mutex				     m_Lock;	   ///< Generic lock
	ShaderLocationRegistryData   m_Data;	   ///< Internally hosted snapshot
	VkGPUValidationCreateInfoAVA m_CreateInfo; ///< Layer create info
};