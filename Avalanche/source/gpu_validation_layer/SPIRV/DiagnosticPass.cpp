#include <SPIRV/DiagnosticPass.h>
#include <DiagnosticRegistry.h>
#include <CRC.h>
#include <StateTables.h>
#include <memory>

using namespace SPIRV;
using namespace spvtools;
using namespace spvtools::opt;

SPIRV::DiagnosticPass::DiagnosticPass(ShaderState * state, const VkPhysicalDeviceProperties2 & properties)
	: Pass(state, "DiagnosticPass")
	, m_Properties(properties)
{
}

spvtools::opt::IRContext::Analysis SPIRV::DiagnosticPass::GetPreservedAnalyses()
{
	return IRContext::kAnalysisDefUse | IRContext::kAnalysisInstrToBlockMapping | IRContext::kAnalysisDecorations | IRContext::kAnalysisCombinators |
		IRContext::kAnalysisNameMap | IRContext::kAnalysisBuiltinVarId | IRContext::kAnalysisConstants;
}

void SPIRV::DiagnosticPass::ReflectSourceExtracts()
{
	SPIRV::ShaderState* state = GetState();

	const VkGPUValidationCreateInfoAVA& create_info = state->m_DeviceDispatchTable->m_CreateInfoAVA;

	analysis::DefUseManager* def_mgr = get_def_use_mgr();

	// Attempt to find source instruction
	get_module()->ForEachInst([&](const Instruction* instr) {
		if (instr->opcode() != SpvOpSource)
			return;

		if (!instr)
		{
			if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
			{
				create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] No source instruction present, source extracts will not be available");
			}
			return;
		}

		if (instr->NumOperands() < 3)
		{
			if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
			{
				create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Source instruction [File, Source] operands have been stripped");
			}
			return;
		}

		const char* file = nullptr;
		std::string preprocessed_source;

		// First non semantic operand may be file or source
		if (instr->GetOperand(2).type == SPV_OPERAND_TYPE_ID)
		{
			Instruction* id_instr = def_mgr->GetDef(instr->GetSingleWordOperand(2));
			file = reinterpret_cast<const char*>(id_instr->GetOperand(1).words.data());
		}
		else
		{
			preprocessed_source = reinterpret_cast<const char*>(instr->GetOperand(2).words.data());
		}

		// Second non semantic operand is always source if present
		if (instr->NumOperands() > 3)
		{
			preprocessed_source = reinterpret_cast<const char*>(instr->GetOperand(3).words.data());
		}

		// May be continued due to SPIRV limitations
		for (auto next = instr->NextNode(); next && next->opcode() == SpvOpSourceContinued; next = next->NextNode())
		{
			preprocessed_source += reinterpret_cast<const char*>(next->GetOperand(0).words.data());
		}

		// Register this file
		// Note that it may be a preprocessed file, up to the frontend compiler
		const std::vector<ShaderLocationMapping>& mappings = state->m_DeviceState->m_DiagnosticRegistry->GetLocationRegistry()->RegisterDXCSourceExtract(
            state->m_DebugName ? state->m_DebugName : "<NoName>",
            file,
		    preprocessed_source.c_str()
		);
		for (auto&& mapping : mappings)
        {
            state->m_SourceFileLUT[mapping.m_Path] = mapping.m_UID;
        }

		// Diagnostic info
		if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
		{
			if (file)
			{
				char buffer[1024];
				FormatBuffer(buffer, "[SPIRV] Found source level information for '%s'", file);
				create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_INFO, __FILE__, __LINE__, buffer);
			}
			else
			{
				char buffer[1024];
				FormatBuffer(buffer, "[SPIRV] Failed to find source level information for '%s'", state->m_DebugName);
				create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
			}
		}

		// Note: This is a hack
		//       The issue is that DXC strips dead resources (as it should) but I have no access to the RTL information at this stage
		if (!preprocessed_source.empty())
		{
			std::string vk_binding = "vk::binding";

			size_t pos = preprocessed_source.find(vk_binding);
			while (pos != std::string::npos)
			{
				uint32_t binding;
				uint32_t set;
				if (sscanf(&preprocessed_source[pos], "vk::binding(%i, %i)", &binding, &set) == 2)
				{
					state->m_LastDescriptorSet = std::max(state->m_LastDescriptorSet, set);

					uint32_t& last = state->m_DescriptorBindingCount[set];
					last = std::max(last, binding + 1);
				}

				pos = preprocessed_source.find(vk_binding, pos + vk_binding.size());
			}
		}
	});
}

spvtools::opt::analysis::Type * SPIRV::DiagnosticPass::FormatToType(VkFormat format)
{
	analysis::TypeManager* type_mgr = context()->get_type_mgr();

	switch (format)
	{
		default:
			return nullptr;

		case VK_FORMAT_R32_UINT:
		{
			spvtools::opt::analysis::Integer type(32, false);
			return type_mgr->GetRegisteredType(&type);
		}

		case VK_FORMAT_R32_SINT:
		{
			spvtools::opt::analysis::Integer type(32, true);
			return type_mgr->GetRegisteredType(&type);
		}

		case VK_FORMAT_R64_UINT:
		{
			spvtools::opt::analysis::Integer type(64, false);
			return type_mgr->GetRegisteredType(&type);
		}

		case VK_FORMAT_R64_SINT:
		{
			spvtools::opt::analysis::Integer type(64, true);
			return type_mgr->GetRegisteredType(&type);
		}
	}
}

uint32_t SPIRV::DiagnosticPass::GetTypeSize(const spvtools::opt::analysis::Type* type)
{
	switch (type->kind())
	{
		default:
			return 0;

		case analysis::Type::kVector:
		{
			const spvtools::opt::analysis::Vector* vec = type->AsVector();
			return GetTypeSize(vec->element_type()) * vec->element_count();
		}
		case analysis::Type::kMatrix:
		{
			const spvtools::opt::analysis::Matrix* mat = type->AsMatrix();
			return GetTypeSize(mat->element_type()) * mat->element_count();
		}
		case analysis::Type::kFloat:
		{
			return type->AsFloat()->width() / 8;
		}
		case analysis::Type::kInteger:
		{
			return type->AsInteger()->width() / 8;
		}
		case analysis::Type::kStruct:
		{
			const spvtools::opt::analysis::Struct* _struct = type->AsStruct();

			uint32_t size = 0;
			for (const spvtools::opt::analysis::Type* elem : _struct->element_types())
			{
				size += GetTypeSize(elem);
			}
			return size;
		}
	}
}

void SPIRV::DiagnosticPass::CleanTypeID(const spvtools::opt::analysis::Type & type)
{
	analysis::TypeManager* type_mgr = context()->get_type_mgr();

	uint32_t id = type_mgr->GetId(&type);
	if (id == 0)
		return;

	type_mgr->RemoveId(id);
}

void SPIRV::DiagnosticPass::EnsureOrdering(uint32_t dependent, uint32_t dependee)
{
	// Note: Disabled for now
#if 0
	spvtools::opt::analysis::DefUseManager* def_mgr = get_def_use_mgr();
	def_mgr->GetDef(dependent)->InsertAfter(def_mgr->GetDef(dependee));
#endif
}

opt::Pass::Status SPIRV::DiagnosticPass::Process()
{
	ReflectSourceExtracts();

	// Ensure capabilities
	context()->AddCapability(std::make_unique<Instruction>(context(), SpvOpCapability, 0, 0, std::initializer_list<Operand>{ Operand(SPV_OPERAND_TYPE_CAPABILITY, { SpvCapabilityAtomicStorageOps }) }));
	context()->AddCapability(std::make_unique<Instruction>(context(), SpvOpCapability, 0, 0, std::initializer_list<Operand>{ Operand(SPV_OPERAND_TYPE_CAPABILITY, { SpvCapabilitySampledBuffer }) }));
	context()->AddCapability(std::make_unique<Instruction>(context(), SpvOpCapability, 0, 0, std::initializer_list<Operand>{ Operand(SPV_OPERAND_TYPE_CAPABILITY, { SpvCapabilityShader }) }));
	
#if 0 /* Reserved for future use */
	context()->AddCapability(std::make_unique<Instruction>(context(), SpvOpCapability, 0, 0, std::initializer_list<Operand>{ Operand(SPV_OPERAND_TYPE_CAPABILITY, { SpvCapabilityVulkanMemoryModelKHR }) }));
#endif 

	// Get the shared states
	SPIRV::ShaderState* state    = GetState();
	analysis::TypeManager* type_mgr = context()->get_type_mgr();
	analysis::DecorationManager* deco_mgr = get_decoration_mgr();
	analysis::DefUseManager* def_mgr  = get_def_use_mgr();

	// Track all registered sets
	get_module()->ForEachInst([&](const Instruction* instr) {
		if (instr->opcode() != SpvOpDecorate)
			return;

		if (instr->GetSingleWordOperand(1) == SpvDecorationDescriptorSet)
		{
			state->m_LastDescriptorSet = std::max(state->m_LastDescriptorSet, instr->GetSingleWordOperand(2));
			state->m_DescriptorSetLUT[instr->GetSingleWordOperand(0)] = instr->GetSingleWordOperand(2);
		}
	});

	// Note: Need to track bindings after the sets as the order is not guarenteed
	get_module()->ForEachInst([&](const Instruction* instr) {
		if (instr->opcode() != SpvOpDecorate)
			return;

		if (instr->GetSingleWordOperand(1) == SpvDecorationBinding)
		{
			uint32_t& last = state->m_DescriptorBindingCount[state->m_DescriptorSetLUT[instr->GetSingleWordOperand(0)]];
			last = std::max(last, instr->GetSingleWordOperand(2) + 1);
		}
	});

	// Insert registry push constant data
	{
		// Get number of pass push constants
		uint32_t pass_pc_count = 0;
		state->m_DeviceState->m_DiagnosticRegistry->EnumeratePushConstants(nullptr, &pass_pc_count);

		// Get push constants
		auto pass_pcs = ALLOCA_ARRAY(SDiagnosticPushConstantInfo, pass_pc_count);
		state->m_DeviceState->m_DiagnosticRegistry->EnumeratePushConstants(pass_pcs, &pass_pc_count);

		// Attempt to find existing push constant variable
		const Instruction* pc_var_instr = nullptr;
		get_module()->ForEachInst([&](const Instruction* instr) {
			if (instr->opcode() != SpvOpVariable)
				return;

			if (instr->GetSingleWordOperand(2) == SpvStorageClassPushConstant)
			{
				pc_var_instr = instr;
			}
		});

		// Register future ids
		state->m_PushConstantVarID = TakeNextId();
		state->m_PushConstantVarTypeID = TakeNextId();

		// Final set of push constant elements
		std::vector<const analysis::Type*> struct_elements;

		// The optional id for decoration cloning
		uint32_t clone_decoration_id = 0;

		// Existing push constant data?
		// If there is we need to recreate it with our own custom data appended and replace all references to it
		uint32_t offset = 0;
		if (pc_var_instr)
		{
			// Get the [registered] variable pointee struct
			const analysis::Struct* struct_ty = type_mgr->GetRegisteredType(type_mgr->GetType(pc_var_instr->GetSingleWordOperand(0)))->AsPointer()->pointee_type()->AsStruct();

			// Copy elements
			struct_elements = struct_ty->element_types();

			// Get the existing decorations
			std::vector<Instruction*> pre_decorations = deco_mgr->GetDecorationsFor(clone_decoration_id = type_mgr->GetId(struct_ty), true);

			// Get the last offset
			for (Instruction* deco : pre_decorations)
			{
				// OpMemberDecorate <sid> <mid> SpvDecorationOffset <offset>
				if (deco->opcode() != SpvOpMemberDecorate || deco->GetSingleWordOperand(2) != SpvDecorationOffset)
					continue;

				uint32_t member = deco->GetSingleWordOperand(1);
				const analysis::Type* member_ty = struct_elements[member];
				
				offset = std::max(offset, deco->GetSingleWordOperand(3) + GetTypeSize(member_ty));
			}

			// Collect all elements
			for (uint32_t i = 0; i < pass_pc_count; i++)
			{
				auto desc = pass_pcs[i];

				// Get type
				analysis::Type* contained = FormatToType(desc.m_Format);
				struct_elements.push_back(contained);

				// Track
				PushConstantState pc_state;
				pc_state.m_ElementIndex = static_cast<uint32_t>(struct_elements.size() - 1);
				pc_state.m_VarTypeID = type_mgr->GetId(contained);
				state->m_RegistryPushConstantLUT[desc.m_UID] = pc_state;
			}
		}
		else
		{
			// Collect all elements
			for (uint32_t i = 0; i < pass_pc_count; i++)
			{
				auto desc = pass_pcs[i];

				// Get type
				analysis::Type* contained = FormatToType(desc.m_Format);
				struct_elements.push_back(contained);

				// Track
				PushConstantState pc_state;
				pc_state.m_ElementIndex = i;
				pc_state.m_VarTypeID = type_mgr->GetId(contained);
				state->m_RegistryPushConstantLUT[desc.m_UID] = pc_state;
			}
		}

		// Get struct type
		analysis::Struct pc_struct(struct_elements);
		CleanTypeID(pc_struct);
		state->m_PushConstantVarTypeID = type_mgr->GetId(type_mgr->GetRegisteredType(&pc_struct));

		// Get push constant pointer
		analysis::Pointer ptr_ty(type_mgr->GetType(state->m_PushConstantVarTypeID), SpvStorageClassPushConstant);
		CleanTypeID(ptr_ty);
		uint32_t ptr_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&ptr_ty));

		// Create global value
		std::unique_ptr<Instruction> newVarOp(new Instruction(context(), SpvOpVariable, ptr_ty_id, state->m_PushConstantVarID, { { spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER, { SpvStorageClassPushConstant } } }));
		context()->AddGlobalValue(std::move(newVarOp));
		EnsureOrdering(ptr_ty_id, state->m_PushConstantVarTypeID);
		EnsureOrdering(state->m_PushConstantVarID, ptr_ty_id);

		// Decorations, we all like to look pretty
		deco_mgr->AddDecoration(state->m_PushConstantVarTypeID, SpvDecorationBlock);
		if (clone_decoration_id)
		{
			deco_mgr->CloneDecorations(clone_decoration_id, state->m_PushConstantVarTypeID);
		}

		// Add pass push offsets
		for (uint32_t i = 0; i < pass_pc_count; i++)
		{
			deco_mgr->AddMemberDecoration(state->m_PushConstantVarTypeID, i + (static_cast<uint32_t>(struct_elements.size()) - pass_pc_count), SpvDecorationOffset, offset);
			offset += FormatToSize(pass_pcs[i].m_Format);
		}

		// Previous existing push constants?
		if (pc_var_instr)
		{
			uint32_t var_id = pc_var_instr->result_id();

			// Replace all usages
			context()->ReplaceAllUsesWith(var_id, state->m_PushConstantVarID);

			// Remove old struct instructions
			def_mgr->GetDef(var_id)->RemoveFromList();
		}
	}

	// Insert registry descriptor data
	{
		// Get number of pass descriptors
		uint32_t pass_descriptor_count = 0;
		state->m_DeviceState->m_DiagnosticRegistry->EnumerateDescriptors(nullptr, &pass_descriptor_count);

		// Get pass descriptors
		auto pass_descriptors = ALLOCA_ARRAY(SDiagnosticDescriptorInfo, pass_descriptor_count);
		state->m_DeviceState->m_DiagnosticRegistry->EnumerateDescriptors(pass_descriptors, &pass_descriptor_count);

		auto state_templates = ALLOCA_ARRAY(DescriptorState, pass_descriptor_count);

		// Create type templates
		for (uint32_t descriptor_idx = 0; descriptor_idx < pass_descriptor_count; descriptor_idx++)
		{
			auto desc = pass_descriptors[descriptor_idx];
			auto&& descriptor_state = state_templates[descriptor_idx];

			switch (desc.m_DescriptorType)
			{
				default:
					// Unsupported feature used by pass
					return Status::Failure;

				/*case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				type = analysis::Image(analysis::geto);
					break;*/

				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				{
					// Note: Storage texel buffers needs to be marked as "UniformConstant"!
					descriptor_state.m_Storage = /*(desc.m_DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) ? SpvStorageClassStorageBuffer :*/ SpvStorageClassUniformConstant;

					// Get texel format
					analysis::Type* texel_format = FormatToType(desc.m_ElementFormat);
					descriptor_state.m_ContainedTypeID = type_mgr->GetId(texel_format);
					descriptor_state.m_Stride = FormatToSize(desc.m_ElementFormat);

					// To image
					analysis::Image image(texel_format, SpvDimBuffer, 2, 0, 0, 2, SpvImageFormatR32ui);
					descriptor_state.m_VarTypeID = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&image));

					// The image instruction adds the access qualifications by default for whatever reason
					// That is a kernel capability and not supported
					Instruction* instr = get_def_use_mgr()->GetDef(descriptor_state.m_VarTypeID);
					if (instr->NumOperands() > 8)
					{
						instr->RemoveOperand(8);
					}
					break;
				}

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				{
					descriptor_state.m_Storage = SpvStorageClassUniform;

					// Get array element format
					analysis::Type* element_type = FormatToType(desc.m_ElementFormat);
					descriptor_state.m_ContainedTypeID = type_mgr->GetId(element_type);

					// To runtime array
					analysis::RuntimeArray element_rarr_ty_tmp(element_type);
					CleanTypeID(element_rarr_ty_tmp);
					analysis::Type* element_rarr_ty_ = type_mgr->GetRegisteredType(&element_rarr_ty_tmp);
					uint32_t element_rarr_ty_id = type_mgr->GetTypeInstruction(element_rarr_ty_);

					// Get safe stride value, very strict requirements...
					descriptor_state.m_Stride = std::max(16u, FormatToSize(desc.m_ElementFormat));
					deco_mgr->AddDecorationVal(element_rarr_ty_id, SpvDecorationArrayStride, descriptor_state.m_Stride);

					// To struct
					analysis::Struct data_ty({ element_rarr_ty_ });
					//CleanTypeID(data_ty);
					descriptor_state.m_VarTypeID = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&data_ty));

					deco_mgr->RemoveDecorationsFrom(descriptor_state.m_VarTypeID);
					deco_mgr->AddDecoration(descriptor_state.m_VarTypeID, SpvDecorationBlock);
					deco_mgr->AddMemberDecoration(descriptor_state.m_VarTypeID, 0, SpvDecorationOffset, 0);
					break;
				}
			}
		}

		// Needs to be inserted for all descriptor sets
		for (uint32_t set_idx = 0; set_idx < state->m_LastDescriptorSet + 1; set_idx++)
		{
			for (uint32_t descriptor_idx = 0; descriptor_idx < pass_descriptor_count; descriptor_idx++)
			{
				auto desc = pass_descriptors[descriptor_idx];
				auto descriptor_state = state_templates[descriptor_idx];

				// Get pointer to descriptor contained type
				analysis::Pointer ptr_ty(type_mgr->GetType(descriptor_state.m_VarTypeID), descriptor_state.m_Storage);
				uint32_t ptr_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&ptr_ty));

				// Create global value
				descriptor_state.m_VarID = TakeNextId();
				std::unique_ptr<Instruction> newVarOp(new Instruction(context(), SpvOpVariable, ptr_ty_id, descriptor_state.m_VarID, { { spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER, { static_cast<uint32_t>(descriptor_state.m_Storage) } } }));
				context()->AddGlobalValue(std::move(newVarOp));

				// Track
				state->m_RegistryDescriptorMergedLUT[desc.m_UID | (set_idx << 16)] = descriptor_state;

				// Assign bindings
				deco_mgr->AddDecorationVal(descriptor_state.m_VarID, SpvDecorationDescriptorSet, set_idx);
				deco_mgr->AddDecorationVal(descriptor_state.m_VarID, SpvDecorationBinding, state->m_DescriptorBindingCount[set_idx] + desc.m_UID);

				EnsureOrdering(ptr_ty_id, descriptor_state.m_VarTypeID);
				EnsureOrdering(descriptor_state.m_VarID, ptr_ty_id);
			}
		}
	}

	// Insert diagnostic data
	{
		// uint32
		analysis::Integer uint_ty(32, false);
		analysis::Type* reg_uint_ty = type_mgr->GetRegisteredType(&uint_ty);
		uint32_t reg_uint_ty_id = type_mgr->GetTypeInstruction(reg_uint_ty);

		// SDiagnosticMessageData
		analysis::Struct message_ty({ reg_uint_ty });
		CleanTypeID(message_ty);
		type_mgr->RemoveId(type_mgr->GetId(&message_ty));
		analysis::Type* reg_message_ty = type_mgr->GetRegisteredType(&message_ty);
		state->m_DataMessageTypeID = type_mgr->GetTypeInstruction(reg_message_ty);

		// SDiagnosticMessageData[]
		analysis::RuntimeArray message_rarr_ty_tmp(reg_message_ty);
		CleanTypeID(message_rarr_ty_tmp);
		analysis::Type* message_rarr_ty_ = type_mgr->GetRegisteredType(&message_rarr_ty_tmp);
		uint32_t message_rarr_ty_id = type_mgr->GetTypeInstruction(message_rarr_ty_);

		// Runtime array decorations
		deco_mgr->AddDecorationVal(message_rarr_ty_id, SpvDecorationArrayStride, 4u);
		deco_mgr->AddMemberDecoration(state->m_DataMessageTypeID, 0, SpvDecorationOffset, 0);

		// SDiagnosticData
		analysis::Struct data_ty({ reg_uint_ty, reg_uint_ty, message_rarr_ty_ });
		CleanTypeID(data_ty);
		analysis::Type* reg_data_ty = type_mgr->GetRegisteredType(&data_ty);
		uint32_t reg_data_ty_id = type_mgr->GetTypeInstruction(reg_data_ty);
		state->m_DataBufferCounterType = reg_uint_ty_id;
		state->m_DataBufferTypeID = reg_data_ty_id;

		// Diagnostics data decorations
		deco_mgr->AddDecoration(reg_data_ty_id, SpvDecorationBlock);
		deco_mgr->AddMemberDecoration(reg_data_ty_id, 0, SpvDecorationOffset, 0);
		deco_mgr->AddMemberDecoration(reg_data_ty_id, 1, SpvDecorationOffset, 4);
		deco_mgr->AddMemberDecoration(reg_data_ty_id, 2, SpvDecorationOffset, 16);
		uint32_t data_ptr_id = type_mgr->FindPointerToType(reg_data_ty_id, SpvStorageClassStorageBuffer);

		// Create global data storage buffer
		state->m_DataBufferVarID = TakeNextId();
		std::unique_ptr<Instruction> newVarOp(new Instruction(context(), SpvOpVariable, data_ptr_id, state->m_DataBufferVarID, { { spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER,{ SpvStorageClassStorageBuffer } } }));
		context()->AddGlobalValue(std::move(newVarOp));

		// Global storage buffer decorations
		deco_mgr->AddDecorationVal(state->m_DataBufferVarID, SpvDecorationDescriptorSet, state->m_LastDescriptorSet + 1);
		deco_mgr->AddDecorationVal(state->m_DataBufferVarID, SpvDecorationBinding, 0);
	}

	// Ensure storage buffer extension
	if (!get_feature_mgr()->HasExtension(kSPV_KHR_storage_buffer_storage_class))
	{
		const std::string ext_name("SPV_KHR_storage_buffer_storage_class");
		const size_t	  num_chars = ext_name.size();

		// Compute num words, accommodate the terminating null character.
		const size_t		  num_words = (num_chars + 1 + 3) / 4;
		std::vector<uint32_t> ext_words(num_words, 0u);
		std::memcpy(ext_words.data(), ext_name.data(), num_chars);

		context()->AddExtension(std::unique_ptr<Instruction>(new Instruction(context(), SpvOpExtension, 0u, 0u, { { SPV_OPERAND_TYPE_LITERAL_STRING, ext_words } })));
	}

	// Ensure atomic ops extension
	if (!get_feature_mgr()->HasExtension(kSPV_KHR_shader_atomic_counter_ops))
	{
		const std::string ext_name("SPV_KHR_shader_atomic_counter_ops");
		const size_t	  num_chars = ext_name.size();

		// Compute num words, accommodate the terminating null character.
		const size_t  	  num_words = (num_chars + 1 + 3) / 4;
		std::vector<uint32_t> ext_words(num_words, 0u);
		std::memcpy(ext_words.data(), ext_name.data(), num_chars);

		context()->AddExtension(std::unique_ptr<Instruction>(new Instruction(context(), SpvOpExtension, 0u, 0u, { { SPV_OPERAND_TYPE_LITERAL_STRING, ext_words } })));
	}

	// Ensure that the extended glsl (ver 450) instruction set is present
	state->m_ExtendedGLSLStd450Set = get_feature_mgr()->GetExtInstImportId_GLSLstd450();
	if (state->m_ExtendedGLSLStd450Set == 0)
	{
		const std::string ext_name("GLSL.std.450");
		const size_t	  num_chars = ext_name.size();

		// Compute num words, accommodate the terminating null character.
		const size_t		  num_words = (num_chars + 1 + 3) / 4;
		std::vector<uint32_t> ext_words(num_words, 0u);
		std::memcpy(ext_words.data(), ext_name.data(), num_chars);

		std::unique_ptr<Instruction> ext_instr(new Instruction(context(), SpvOpExtInstImport, 0u, TakeNextId(), { { SPV_OPERAND_TYPE_LITERAL_STRING, ext_words } }));
		state->m_ExtendedGLSLStd450Set = ext_instr->result_id();
		get_module()->AddExtInstImport(std::move(ext_instr));
	}

	context()->BuildInvalidAnalyses(IRContext::kAnalysisDefUse);

	return Status::SuccessWithChange;
}
