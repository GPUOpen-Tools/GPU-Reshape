#pragma once

#include "Pass.h"
#include <DiagnosticData.h>

namespace SPIRV 
{
	class DiagnosticPass : public Pass
	{
	public:
		DiagnosticPass(ShaderState* state, const VkPhysicalDeviceProperties2& properties);

		/// Overrides
		spvtools::opt::IRContext::Analysis GetPreservedAnalyses() override;
		Status Process() override;

	private:
		/**
		 * Reflect all source level information
		 */
		void ReflectSourceExtracts();

		/**
		 * Convert a vulkan format to a type
		 * @param[in] format the vulkan format
		 */
		spvtools::opt::analysis::Type* FormatToType(VkFormat format);

		/**
		 * Get the expected size of a type
		 * ! Must be POD
		 * @param[in] format the vulkan format
		 */
		uint32_t GetTypeSize(const spvtools::opt::analysis::Type* type);

		/**
		 * Clean a type id from the type manager
		 * @param[in] type the type to clean
		 */
		void CleanTypeID(const spvtools::opt::analysis::Type& type);

		/**
		 * Ensure the ordering of two dependent instructions
		 * @param[in] dependent the source instruction
		 * @param[in] dependee the dependency
		 */
		void EnsureOrdering(uint32_t dependent, uint32_t dependee);

	private:
		VkPhysicalDeviceProperties2 m_Properties;
	};
}
