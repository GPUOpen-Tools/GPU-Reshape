#pragma once

#include "Common.h"
#include <vector>
#include <string>

struct HSourceShader
{
	std::vector<uint8_t>	 m_Blob;
	VkShaderModuleCreateInfo m_CreateInfo{};
	VkShaderModule			 m_Module = nullptr;
	std::string				 m_Name;
};

struct HInstrumentedShader
{
	std::vector<uint32_t>	 m_SpirvCache;
	VkShaderModuleCreateInfo m_CreateInfo{};
	VkShaderModule			 m_Module = nullptr;
};
