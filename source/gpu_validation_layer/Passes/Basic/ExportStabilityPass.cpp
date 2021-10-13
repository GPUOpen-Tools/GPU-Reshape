#include <Passes/Basic/ExportStabilityPass.h>
#include <SPIRV/InjectionPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>
#include <DispatchTables.h>
#include <CRC.h>

using namespace Passes;
using namespace Passes::Basic;

struct ExportStabilityValidationMessage
{
	uint32_t m_Type           :  3;
	uint32_t m_ErrorType      :  2;
	uint32_t m_ShaderSpanGUID : kShaderLocationGUIDBits;
	uint32_t m_DeadBeef		  :  (kMessageBodyBits - kShaderLocationGUIDBits - 3 - 2);
};

class ExportStabilitySPIRVPass : public SPIRV::InjectionPass
{
public:
	ExportStabilitySPIRVPass(DiagnosticRegistry* registry, SPIRV::ShaderState* state, uint16_t error_uid)
		: InjectionPass(state, "ExportStabilityPass")
		, m_Registry(registry)
		, m_ErrorUID(error_uid)
	{
	}

	void Validate(spvtools::opt::InstructionBuilder& builder, uint32_t& state_mask_id, const spvtools::opt::analysis::Type* type, spvtools::opt::Instruction* value)
	{
		using namespace spvtools;
		using namespace spvtools::opt;

		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Integer int_ty(32, false);
		uint32_t uint_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&int_ty));

		switch (type->kind())
		{
			default:
				break;
			case analysis::Type::kStruct:
			{
				auto&& element_types = type->AsStruct()->element_types();

				// Validate all elements
				for (size_t i = 0; i < element_types.size(); i++)
				{
					Validate(builder, state_mask_id, element_types[i], builder.AddCompositeExtract(type_mgr->GetId(element_types[i]), value->result_id(), { static_cast<uint32_t>(i) }));
				}
				break;
			}
			case analysis::Type::kVector:
			case analysis::Type::kInteger:
			case analysis::Type::kFloat:
			{
				analysis::Bool bool_ty;
				uint32_t	   bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

				// TODO: Non FP validation?
				bool is_fp = (type->kind() == analysis::Type::kFloat);

				uint32_t is_ruid = bool_ty_id;
				if (type->kind() == analysis::Type::kVector)
				{
					analysis::Vector bool_vec_ty(&bool_ty, type->AsVector()->element_count());
					is_ruid = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_vec_ty));

					is_fp = (type->AsVector()->element_type()->kind() == analysis::Type::kFloat);
				}

				// Nan
				if (is_fp) 
				{
					Instruction* is_nan = builder.AddInstruction(AllocInstr(SpvOpIsNan, is_ruid,
					{
						{ SPV_OPERAND_TYPE_ID, { value->result_id() } }
					}));

					// is_nan = any(is_nan)
					if (type->kind() == analysis::Type::kVector)
					{
						is_nan = builder.AddInstruction(AllocInstr(SpvOpAny, bool_ty_id, { Operand(SPV_OPERAND_TYPE_ID, { is_nan->result_id() }) }));
					}

					// Expand width
					Instruction* expanded = builder.AddInstruction(AllocInstr(SpvOpSelect, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { is_nan->result_id() } },
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(static_cast<uint32_t>(ExportStabilityValidationErrorType::eNaN)) } },
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) } }
					}));

					// Bitwise-or the flag
					state_mask_id = builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { state_mask_id } },
						{ SPV_OPERAND_TYPE_ID, { expanded->result_id() } }
					}))->result_id();
				}

				// Inf
				if (is_fp)
				{
					Instruction* is_inf = builder.AddInstruction(AllocInstr(SpvOpIsInf, is_ruid,
					{
						{ SPV_OPERAND_TYPE_ID, { value->result_id() } }
					}));

					// is_nan = any(is_nan)
					if (type->kind() == analysis::Type::kVector)
					{
						is_inf = builder.AddInstruction(AllocInstr(SpvOpAny, bool_ty_id, { Operand(SPV_OPERAND_TYPE_ID, { is_inf->result_id() }) }));
					}

					// Expand width
					Instruction* expanded = builder.AddInstruction(AllocInstr(SpvOpSelect, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { is_inf->result_id() } },
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(static_cast<uint32_t>(ExportStabilityValidationErrorType::eInf)) } },
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) } }
					}));

					// Bitwise-or the flag
					state_mask_id = builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { state_mask_id } },
						{ SPV_OPERAND_TYPE_ID, { expanded->result_id() } }
					}))->result_id();
				}
				break;
			}
		}
	}

	void Inject(spvtools::opt::BasicBlock* block, spvtools::opt::analysis::Type* type, spvtools::opt::BasicBlock::iterator iit, spvtools::opt::Instruction* ret_value)
	{
		using namespace spvtools;
		using namespace spvtools::opt;

		if (m_InjectedExports.count(ret_value) || IsInjectedInstruction(ret_value))
			return;

		m_InjectedExports.insert(ret_value);

		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Bool bool_ty;
		uint32_t	   bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

		analysis::Integer uint_ty(32, false);
		uint32_t uint_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&uint_ty));

		Instruction source_instr = *iit;

		analysis::ConstantManager* const_mgr = context()->get_constant_mgr();

		// Attempt to find source extract
		uint32_t source_extract_guid = FindSourceExtractGUID(block, iit);

		// Create blocks
		// ... start ...
		//   BrCond Post Error
		// Error:
		//   WriteMessage
		//   Br Post
		// Post:
		//   ReturnValue
		BasicBlock* error_block = AllocBlock(block, true);
		BasicBlock* post_block = SplitBasicBlock(block, iit, false);

		// Pre-export, validate all exported data
		// Jump to error or post depending on state
		uint32_t state_mask;
		{
			InstructionBuilder builder(context(), block);

			state_mask = const_mgr->GetDefiningInstruction(const_mgr->GetConstant(&uint_ty, { 0 }))->result_id();

			// Validate the value
			Validate(builder, state_mask, type, ret_value);

			// State mask not zero?
			Instruction* not_zero = builder.AddInstruction(AllocInstr(SpvOpINotEqual, bool_ty_id,
			{
				{ SPV_OPERAND_TYPE_ID, { state_mask } },
				{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) } },
			}));

			// flag:true denotes an error
			builder.AddConditionalBranch(not_zero->result_id(), error_block->id(), post_block->id());
		}

		// Error block, write message
		// Jump to post
		{
			InstructionBuilder builder(context(), error_block);

			// Compose error message
			{
				// Shift mask left
				uint32_t mask_shl3 = builder.AddInstruction(AllocInstr(SpvOpShiftLeftLogical, uint_ty_id,
				{
					{ SPV_OPERAND_TYPE_ID, { state_mask } },
					{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(3) } },
				}))->result_id();

				// Shift shader uid left
				uint32_t shader_uid_shl5 = builder.GetUintConstantId(source_extract_guid << 5);

				// Composite message
				uint32_t message_id = builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
				{
					{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(static_cast<uint32_t>(ExportStabilityValidationModel::eFragment)) } },
					{ SPV_OPERAND_TYPE_ID, { 
						builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
						{
							{ SPV_OPERAND_TYPE_ID, { mask_shl3 } },
							{ SPV_OPERAND_TYPE_ID, { shader_uid_shl5 } },
						}))->result_id()
					} },
				}))->result_id();

				// Export the message
				ExportMessage(builder, CompositeDynamicMessage(builder, builder.GetUintConstantId(m_ErrorUID), message_id));
			}

			builder.AddBranch(post_block->GetLabel()->result_id());
		}

	}

	bool Visit(spvtools::opt::BasicBlock* block) override
	{
		using namespace spvtools;
		using namespace spvtools::opt;

		analysis::DefUseManager* def_mgr = get_def_use_mgr();
		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		SPIRV::ShaderState* state = GetState();

        const VkGPUValidationCreateInfoAVA& create_info = state->m_DeviceDispatchTable->m_CreateInfoAVA;
		
		auto entry_point = get_module()->entry_points().begin();

		// Skip any blocks not part of the main function
		bool is_entry_point = (entry_point->GetSingleWordOperand(1) == block->GetParent()->result_id());

		for (auto iit = block->begin(); iit != block->end(); ++iit)
		{
			switch (iit->opcode())
			{
				default:
					break;

				case SpvOpReturnValue:
				{
				    if (!is_entry_point)
				        break;

					switch (static_cast<SpvExecutionModel>(entry_point->GetSingleWordInOperand(0)))
					{
						default:
							break;

						case SpvExecutionModelFragment:
						{
							//Inject(block, iit);
							return true;
						}
					}
					break;
				}

			    case SpvOpImageWrite:
                {
                    uint32_t image_id = iit->GetSingleWordOperand(0);

                    // Find originating image type
                    Instruction* image = FindDeclarationType(image_id);
                    if (!image)
                    {
                        if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
                        {
                            create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Failed to find image operand declaration, skipping instruction instrumentation");
                        }
                        break;
                    }

                    // Get image component type
                    analysis::Type* component_type = type_mgr->GetType(image->GetSingleWordOperand(1));

                    // Get image format
                    auto format = static_cast<SpvImageFormat>(image->GetSingleWordOperand(7));

                    // To texel type
                    uint32_t component_count = 0;
                    switch (format)
                    {
                        default:
                            break;
                        case SpvImageFormatRgba32f:
                        case SpvImageFormatRgba8:
                        case SpvImageFormatRgba16:
                        case SpvImageFormatRgba16Snorm:
                        case SpvImageFormatRgba32i:
                        case SpvImageFormatRgba16i:
                        case SpvImageFormatRgba8i:
                        case SpvImageFormatRgba8Snorm:
                        case SpvImageFormatRgba32ui:
                        case SpvImageFormatRgba16ui:
                        case SpvImageFormatRgba8ui:
                            component_count = 4;
                            break;
                        case SpvImageFormatRgb10A2:
                        case SpvImageFormatRgb10a2ui:
                        case SpvImageFormatR11fG11fB10f:
                            component_count = 3;
                            break;
                        case SpvImageFormatRg32f:
                        case SpvImageFormatRg16f:
                        case SpvImageFormatRg16:
                        case SpvImageFormatRg8:
                        case SpvImageFormatRg16Snorm:
                        case SpvImageFormatRg8Snorm:
                        case SpvImageFormatRg32i:
                        case SpvImageFormatRg16i:
                        case SpvImageFormatRg8i:
                        case SpvImageFormatRg32ui:
                        case SpvImageFormatRg16ui:
                        case SpvImageFormatRg8ui:
                            component_count = 2;
                            break;
                        case SpvImageFormatR32f:
                        case SpvImageFormatR16f:
                        case SpvImageFormatR16:
                        case SpvImageFormatR8:
                        case SpvImageFormatR16Snorm:
                        case SpvImageFormatR8Snorm:
                        case SpvImageFormatR32i:
                        case SpvImageFormatR16i:
                        case SpvImageFormatR8i:
                        case SpvImageFormatR32ui:
                        case SpvImageFormatR16ui:
                        case SpvImageFormatR8ui:
                            component_count = 1;
                            break;
                    }

                    // Unsupported?
                    if (!component_count)
                    {
                        if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
                        {
                            create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Unexpected OpTypeImage format operand, skipping instruction instrumentation");
                        }
                        break;
                    }

                    // To texel type
                    analysis::Type* texel_type = component_type;
                    if (component_count > 1)
                    {
                        analysis::Vector vector_type(component_type, component_count);
                        texel_type = type_mgr->GetRegisteredType(&vector_type);
                    }

                    // Texel value
                    Instruction* value = def_mgr->GetDef(iit->GetSingleWordOperand(2));

                    // Safe to assume an export operation at this point
                    Inject(block, texel_type, iit, value);
                    return true;
                }

				case SpvOpStore:
				{
                    if (!is_entry_point)
                        break;

                    switch (static_cast<SpvExecutionModel>(entry_point->GetSingleWordInOperand(0)))
					{
						default:
							break;

						case SpvExecutionModelFragment:
						{
							Instruction* var = def_mgr->GetDef(iit->GetSingleWordOperand(0));
							Instruction* value = def_mgr->GetDef(iit->GetSingleWordOperand(1));

							// Non variables are not outputs AFAIK
							if (var->opcode() != SpvOpVariable)
								continue;

							// Output denotes exports
							auto storage = static_cast<SpvStorageClass>(var->GetSingleWordOperand(2));
							if (storage != SpvStorageClassOutput)
								continue;

							// Get non pointer type
							analysis::Type* type = type_mgr->GetRegisteredType(type_mgr->GetType(var->GetSingleWordOperand(0))->AsPointer()->pointee_type());

							// Safe to assume an export operation at this point
							Inject(block, type, iit, value);
							return true;
						}
					}
					break;
				}
			}
		}

		return true;
	}

private:
	DiagnosticRegistry* m_Registry;
	uint16_t			m_ErrorUID;

	std::set<spvtools::opt::Instruction*> m_InjectedExports;
};

ExportStabilityPass::ExportStabilityPass(DeviceDispatchTable* table, DeviceStateTable* state) : m_Table(table), m_State(state)
{
	m_ErrorUID = m_State->m_DiagnosticRegistry->AllocateMessageUID();

    m_State->m_DiagnosticRegistry->SetMessageHandler(m_ErrorUID, this);
}

void ExportStabilityPass::Initialize(VkCommandBuffer cmd_buffer)
{
}

void ExportStabilityPass::Release()
{
}

uint32_t ExportStabilityPass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage)
{
	uint32_t handled = 0;

	uint64_t batch_key = 0;
	uint32_t batch_count = 0;

	for (uint32_t i = 0; i < count; i++)
	{
		auto& msg = messages[i];

		// Next batch?
		if (msg.GetKey() != batch_key)
		{
			if (batch_count)
			{
				InsertBatched(message_cache, version, batch_key, messages[i - 1], batch_count);
				handled += batch_count;

				batch_count = 0;
			}

			batch_key = msg.GetKey();
		}

		batch_count++;
	}

	// Dangling batch
	if (batch_count != 0)
	{
		InsertBatched(message_cache, version, messages[count - 1].GetKey(), messages[count - 1], batch_count);
		handled += batch_count;
	}

	return handled;
}

void ExportStabilityPass::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	optimizer->RegisterPass(SPIRV::CreatePassToken<ExportStabilitySPIRVPass>(m_State->m_DiagnosticRegistry.get(), state, m_ErrorUID));
}

void ExportStabilityPass::Step(VkGPUValidationReportAVA report)
{
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_EXPORT_UNSTABLE] += m_AccumulatedStepMessages;

	m_AccumulatedStepMessages = 0;
}

void ExportStabilityPass::Report(VkGPUValidationReportAVA report)
{
	report->m_Messages.insert(report->m_Messages.end(), m_Messages.begin(), m_Messages.end());
}

void ExportStabilityPass::Flush()
{
	m_Messages.clear();
	m_MessageLUT.clear();
	m_AccumulatedStepMessages = 0;
}

void ExportStabilityPass::InsertBatched(SStringCache *message_cache, SCommandBufferVersion &version, uint64_t key, const SDiagnosticMessageData & message, uint32_t count)
{
	m_AccumulatedStepMessages += count;

	// Merge if possible
	auto it = m_MessageLUT.find(key);
	if (it != m_MessageLUT.end())
	{
		m_Messages[it->second].m_MergedCount += count;
		return;
	}

	VkGPUValidationMessageAVA msg{};
	msg.m_Type = VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA;
	msg.m_MergedCount = count;
	msg.m_Feature = VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY;
	msg.m_Error.m_ObjectInfo.m_Name = nullptr;
	msg.m_Error.m_ObjectInfo.m_Object = nullptr;
	msg.m_Error.m_UserMarkerCount = 0;

	// Import message
	auto imported = message.GetMessage<ExportStabilityValidationMessage>();

    if (imported.m_ShaderSpanGUID != UINT32_MAX && m_State->m_DiagnosticRegistry->GetLocationRegistry()->GetExtractFromUID(imported.m_ShaderSpanGUID, &msg.m_Error.m_SourceExtract))
    {
		// TODO: Render pass state extraction!
	}

    // Compose validation message
    {
        char buffer[256]{0};
        switch (static_cast<ExportStabilityValidationModel>(imported.m_Type))
        {
            case ExportStabilityValidationModel::eCount:
                break;
            case ExportStabilityValidationModel::eFragment:
                FormatBuffer(buffer, "Fragment export is");
                break;
        }

        uint32_t state_counter = 0;
        if (imported.m_ErrorType & static_cast<uint32_t>(ExportStabilityValidationErrorType::eNaN))
            FormatBuffer(buffer, state_counter++ ? "%s & NaN" : "%s NaN", buffer);

        if (imported.m_ErrorType & static_cast<uint32_t>(ExportStabilityValidationErrorType::eInf))
            FormatBuffer(buffer, state_counter++ ? "%s & Inf" : "%s Inf", buffer);

        // Copy to cache
        msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_EXPORT_UNSTABLE;
        msg.m_Error.m_Message = message_cache->Get(buffer);
    }

	m_Messages.push_back(msg);
	m_MessageLUT.insert(std::make_pair(key, m_Messages.size() - 1));
}
