#pragma once

#include <Common.h>
#include <spirv-tools/opt/ir_builder.h>
#include <spirv-tools/opt/pass.h>
#include <spirv-tools/optimizer.hpp>
#include <spirv-tools/instrument.hpp>
#include <unordered_map>
#include <string>

struct DeviceDispatchTable;
struct DeviceStateTable;

namespace SPIRV
{
	// Represents a SPIRV pass descriptor state
	struct DescriptorState
	{
		SpvStorageClass m_Storage;
		uint32_t m_ContainedTypeID;
		uint32_t m_VarID;
		uint32_t m_VarTypeID;
		uint32_t m_Stride;
	};
	
	// Represents a SPIRV pass push constant state
	struct PushConstantState
	{
		uint32_t m_VarTypeID;
		uint32_t m_ElementIndex;
	};

	// A collection of shared data during a shader recompilation
	struct ShaderState
	{
		DeviceStateTable*    m_DeviceState;         ///< The device state table
		DeviceDispatchTable* m_DeviceDispatchTable; ///< The device dispatch table
		const char*			 m_DebugName;			///< The extension provided debug name of this shader

		uint32_t m_DataBufferVarID;       ///< The diagnostics data buffer variable id
		uint32_t m_DataBufferCounterType; ///< The diagnostics data buffer atomic counter type
		uint32_t m_DataBufferTypeID;      ///< The diagnostics data buffer [non pointer] variable type id
		uint32_t m_DataMessageTypeID;     ///< The contained diagnostics message type id
		uint32_t m_PushConstantVarID;	  ///< The in-stage push constant variable id
		uint32_t m_PushConstantVarTypeID; ///< The in-stage push constant [non pointer] variable type id

		uint32_t m_ExtendedGLSLStd450Set; ///< The id of the extended glsl instruction set [ver 450]

		std::unordered_set<uint32_t>				          m_UserLabelResultIDs;			 ///< All user labels
		std::unordered_set<const spvtools::opt::Instruction*> m_UserLocalInstructionIDs;	 ///< All locally instrumented instructions
		std::unordered_map<std::string, uint16_t>	          m_SourceFileLUT;				 ///< File to file uid lookup
		std::unordered_map<uint32_t, uint32_t>		          m_DescriptorSetLUT;			 ///< Declaration to descriptor set lookup
		std::unordered_map<uint64_t, DescriptorState>         m_RegistryDescriptorMergedLUT; ///< Merged (uid | (set << 16)) to descriptor state lookup
		std::unordered_map<uint16_t, PushConstantState>       m_RegistryPushConstantLUT;	 ///< Push constant uid to push constant state lookup

		uint32_t m_LastDescriptorSet = 0;			///< Last user descriptor set
		uint32_t m_DescriptorBindingCount[32]{ 0 }; ///< The number of bindings within all descriptor sets
	};

	class Pass : public spvtools::opt::Pass
	{
	public:
		Pass(ShaderState* state, const char* name);

		/// Overrides
		const char* name() const override;

		/**
		 * Get the shared shader state
		 */
		ShaderState* GetState() 
		{
			return m_State; 
		}

	private:
		ShaderState* m_State; ///< The shared shader state
		const char*  m_Name;  ///< The name of this pass
	};

	/**
	 * Create a new pass token
	 * @param[in] args the construction arguments
	 */
	template<typename T, typename... A>
	spvtools::Optimizer::PassToken CreatePassToken(A&&... args)
	{
		return spvtools::Optimizer::PassToken(std::make_unique<T>(args...));
	}
}