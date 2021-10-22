#include <Passes/Basic/ResourceBoundsPass.h>
#include <SPIRV/InjectionPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>
#include <DispatchTables.h>
#include <CRC.h>

using namespace Passes;
using namespace Passes::Basic;

struct ResourceBoundsValidationMessage
{
	uint32_t m_ResourceType   :  1;
	uint32_t m_ShaderSpanGUID : kShaderLocationGUIDBits;
	uint32_t m_DeadBeef	      :  (kMessageBodyBits - kShaderLocationGUIDBits - 1);
};

class ResourceBoundsSPIRVPass : public SPIRV::InjectionPass
{
public:
	ResourceBoundsSPIRVPass(DiagnosticRegistry* registry, SPIRV::ShaderState* state, uint16_t error_uid)
		: InjectionPass(state, "ResourceBoundsPass")
		, m_Registry(registry)
		, m_ErrorUID(error_uid)
	{
	}

	bool Visit(spvtools::opt::BasicBlock* block) override
	{
		using namespace spvtools;
		using namespace spvtools::opt;

		SPIRV::ShaderState* state = GetState();

		const VkGPUValidationCreateInfoAVA& create_info = state->m_DeviceDispatchTable->m_CreateInfoAVA;

		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Integer uint_ty(32, false);
		analysis::Type* reg_uint_ty = type_mgr->GetRegisteredType(&uint_ty);
		uint32_t reg_uint_ty_id = type_mgr->GetTypeInstruction(reg_uint_ty);

		analysis::Bool bool_ty;
		uint32_t bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

		analysis::DefUseManager* def_mgr = get_def_use_mgr();

		for (auto iit = block->begin(); iit != block->end(); ++iit)
		{
			// Copied as it may be moved
			Instruction source_instr = *iit;

			switch (source_instr.opcode())
			{
				default: 
					break;

				/* Same instruction specification for the relevant arguments */
				case SpvOpImageWrite:
				case SpvOpImageFetch:
				case SpvOpImageRead:
				{
					// Already instrumented?
					if (m_InstrumentedResults.count(&*iit) || IsInjectedInstruction(&*iit))
						continue;

					m_InstrumentedResults.insert(&*iit);

					uint32_t image_id = source_instr.GetSingleWordInOperand(0);
					uint32_t addr_id  = source_instr.GetSingleWordInOperand(1);

					// Find the declaration
					Instruction* image_decl = FindDeclaration(image_id);
                    if (!image_decl)
                    {
                        if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
                        {
                            create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Failed to find image operand declaration, skipping instruction instrumentation");
                        }
                        continue;
                    }

					// Find originating image type
					Instruction* image = FindDeclarationType(image_id);
                    if (!image)
                    {
                        if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
                        {
                            create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Failed to find image operand declaration, skipping instruction instrumentation");
                        }
                        continue;
                    }

					auto image_dim = static_cast<SpvDim>(image->GetSingleWordOperand(2));

					// Sampled images have additional constraints
					uint32_t image_sampled_word = image->GetSingleWordOperand(6);

					// Deduce the bottom level addressing dimensions
					uint32_t image_address_dimensions;
					switch (image_dim)
					{
					default:
						continue;
					case SpvDim1D:
					case SpvDim2D:
					case SpvDim3D:
						image_address_dimensions = image->GetSingleWordOperand(2) + 1;
						break;
					case SpvDimCube:
						image_address_dimensions = 3;
						break;
					case SpvDimBuffer:
						// TODO: The SPIRV specification is a bit rough on this... Seems to work though!
						image_address_dimensions = 1;
						image_sampled_word = 0;
						break;
					}

					// Assumes uniform source instruction id
					uint32_t routed_result_id = source_instr.result_id();

					// As the offending block execution is optional we need to respect the domination of the result id
					// This is achieved by routing it through a PHI node when merging
					bool needs_phi_routing = (source_instr.opcode() == SpvOpImageFetch) || (source_instr.opcode() == SpvOpImageRead);
					if (needs_phi_routing)
					{
						iit->SetResultId(routed_result_id = TakeNextId());
					}

					// Ensure capabilities
					context()->AddCapability(std::make_unique<Instruction>(context(), SpvOpCapability, 0, 0, std::initializer_list<Operand>{ Operand(SPV_OPERAND_TYPE_CAPABILITY, { SpvCapabilityImageQuery }) }));

					// The image component can be determined by the address
					Instruction* image_addr_ty_id = def_mgr->GetDef(addr_id);
					analysis::Type* image_addr_ty	= type_mgr->GetType(image_addr_ty_id->GetSingleWordOperand(0));

					// Attempt to find source extract
					uint32_t source_extract_guid = FindSourceExtractGUID(block, iit);
                    if (source_extract_guid != UINT32_MAX)
                    {
                        ShaderLocationBinding binding{};

                        // Attempt to get bindings
                        // If present register the mapping
                        if (GetDescriptorBinds(image_decl->result_id(), &binding.m_SetIndex, &binding.m_BindingIndex))
                        {
                            m_Registry->GetLocationRegistry()->RegisterExtractBinding(source_extract_guid, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS, binding);
                        }
                    }

					// Size query dimensions
					uint32_t query_dimensions = image_address_dimensions;

					// Arrayed image?
					// Adds implicit addressing dimension
					bool arrayed = (image->GetSingleWordOperand(4) == 1);
					if (arrayed)
					{
						query_dimensions++;
					}

					// Check for additional lod dimension
					bool lod_addr = false;
					if (source_instr.opcode() == SpvOpImageFetch && image_addr_ty->kind() == analysis::Type::kVector)
					{
						lod_addr = (image_addr_ty->AsVector()->element_count() > query_dimensions);
					}

					// Create blocks
					// ... start ...
					//   BrCond Offending Error
					// Offending:
					//   OpImageWrite
					//   Br Post
					// Post:
					// SSA PHI
					//  ... end ...
					// Error:
					//   WriteMessage
					//   Br Post
					BasicBlock* offending_block = SplitBasicBlock(block, iit, false);
					BasicBlock* post_block = SplitBasicBlock(offending_block, ++offending_block->begin(), false);
					BasicBlock* error_block = AllocBlock(block, true);

					// The offending block just branches to the post
					{
						InstructionBuilder builder(context(), offending_block);
						builder.AddBranch(post_block->GetLabel()->result_id());
					}

					// The base block validates the addresses, if OOB jumps to error otherwise offending
					{
						InstructionBuilder builder(context(), block);

						uint32_t size_uid = reg_uint_ty_id;
						if (image_addr_ty->kind() == analysis::Type::kVector)
						{
							analysis::Vector size_vec_ty(reg_uint_ty, query_dimensions);
							size_uid = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&size_vec_ty));
						}

						// size = ImageQuerySize(image)
						Instruction* query;
						switch (image_sampled_word)
						{
							// SpvOpImageQuerySize requires 0 or 2, 1 denotes a sampled image
							case 1:
							{
								// Determine the appropriate lod level, may be implicit
								uint32_t lod_uid;
								if (lod_addr)
									lod_uid = Track(builder.AddCompositeExtract(type_mgr->GetId(image_addr_ty->AsVector()->element_type()), addr_id, { image_addr_ty->AsVector()->element_count() - 1 }))->result_id();
								else
									lod_uid = builder.GetUintConstantId(0);

								query = builder.AddInstruction(AllocInstr(SpvOpImageQuerySizeLod, size_uid, { Operand(SPV_OPERAND_TYPE_IMAGE, { image_id }), Operand(SPV_OPERAND_TYPE_ID, { lod_uid }) }));
								break;
							}
							default:
								query = builder.AddInstruction(AllocInstr(SpvOpImageQuerySize, size_uid, { Operand(SPV_OPERAND_TYPE_IMAGE, { image_id }) }));
								break;
						}

						uint32_t oob_ruid = bool_ty_id;
						if (image_addr_ty->kind() == analysis::Type::kVector)
						{
							analysis::Vector bool_vec_ty(&bool_ty, query_dimensions);
							oob_ruid = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_vec_ty));
						}

						// Get rid of lod index if needed
						uint32_t safe_addr_id = addr_id;
						if (lod_addr)
						{
							// Blame SPIRV-Tools for this
							std::vector<uint32_t> indices;
							for (uint32_t i = 0; i < query_dimensions; i++) indices.push_back(i);

							analysis::Vector shaved_addr_ty(type_mgr->GetRegisteredType(image_addr_ty->AsVector()->element_type()), query_dimensions);
							uint32_t shaved_addr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&shaved_addr_ty));

							// Extract
							safe_addr_id = Track(builder.AddCompositeExtract(shaved_addr_ty_id, addr_id, indices))->result_id();
						}

						// oob = address > size
						Instruction* state_vec = Track(builder.AddBinaryOp(oob_ruid, SpvOpUGreaterThanEqual, safe_addr_id, query->result_id()));

						// oob = any(oob)
						if (image_addr_ty->kind() == analysis::Type::kVector)
						{
							state_vec = builder.AddInstruction(AllocInstr(SpvOpAny, bool_ty_id, { Operand(SPV_OPERAND_TYPE_ID, { state_vec->result_id() }) }));
						}

						// oob ? error : offending
						Track(builder.AddConditionalBranch(state_vec->result_id(), error_block->GetLabel()->result_id(), offending_block->GetLabel()->result_id()));
					}

					// The error block writes error data and jumps to post
					{
						InstructionBuilder builder(context(), error_block);

						// Compose error message
						{
							ResourceBoundsValidationMessage message;
							message.m_ShaderSpanGUID = source_extract_guid;
							message.m_ResourceType = static_cast<uint32_t>((image_dim == SpvDim::SpvDimBuffer) ? ResourceBoundsValidationResourceType::eBuffer : ResourceBoundsValidationResourceType::eImage);
							message.m_DeadBeef = 0;

							ExportMessage(builder, CompositeStaticMessage(builder, SDiagnosticMessageData::Construct(m_ErrorUID, message)));
						}

						builder.AddBranch(post_block->GetLabel()->result_id());
					}

					// The post block needs to deduce the correct result value
					if (needs_phi_routing)
					{
						analysis::ConstantManager* const_mgr = context()->get_constant_mgr();
						analysis::Type* result_ty = type_mgr->GetType(source_instr.GetSingleWordOperand(0));

						// Void operand list denotes null constant
						Instruction* null_instr = const_mgr->GetDefiningInstruction(const_mgr->GetConstant(result_ty, {}));

						// Select value based on previous control flow
						auto select = std::make_unique<spvtools::opt::Instruction>(
							context(), SpvOpPhi, source_instr.GetSingleWordOperand(0), source_instr.result_id(),
							Instruction::OperandList
							{
								Operand(SPV_OPERAND_TYPE_ID, { routed_result_id  }), Operand(SPV_OPERAND_TYPE_ID, { offending_block->id() }), // Offending block
								Operand(SPV_OPERAND_TYPE_ID, { null_instr->result_id() }), Operand(SPV_OPERAND_TYPE_ID, { error_block->id()     }), // Error block
							}
						);

						Track(select.get());
						post_block->begin().InsertBefore(std::move(select));
					}

					return true;
				}
			}
		}

		return true;
	}

private:
	DiagnosticRegistry* m_Registry;
	uint16_t			m_ErrorUID;

	std::unordered_set<spvtools::opt::Instruction*> m_InstrumentedResults;
};

ResourceBoundsPass::ResourceBoundsPass(DeviceDispatchTable* table, DeviceStateTable* state) : m_Table(table), m_State(state)
{
	m_ErrorUID = m_State->m_DiagnosticRegistry->AllocateMessageUID();

    m_State->m_DiagnosticRegistry->SetMessageHandler(m_ErrorUID, this);
}

void ResourceBoundsPass::Initialize(VkCommandBuffer cmd_buffer)
{
}

void ResourceBoundsPass::Release()
{
}

uint32_t ResourceBoundsPass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage)
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
				InsertBatched(version, batch_key, messages[i - 1], batch_count);
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
		InsertBatched(version, messages[count - 1].GetKey(), messages[count - 1], batch_count);
		handled += batch_count;
	}

	return handled;
}

void ResourceBoundsPass::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	optimizer->RegisterPass(SPIRV::CreatePassToken<ResourceBoundsSPIRVPass>(m_State->m_DiagnosticRegistry.get(), state, m_ErrorUID));
}

void ResourceBoundsPass::Step(VkGPUValidationReportAVA report)
{
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_IMAGE_OVERFLOW_AVA] += m_AccumulatedStepMessages[0];
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA] += m_AccumulatedStepMessages[1];

	m_AccumulatedStepMessages[0] = 0;
	m_AccumulatedStepMessages[1] = 0;
}

void ResourceBoundsPass::Report(VkGPUValidationReportAVA report)
{
	report->m_Messages.insert(report->m_Messages.end(), m_Messages.begin(), m_Messages.end());
}

void ResourceBoundsPass::Flush()
{
	m_Messages.clear();
	m_MessageLUT.clear();
	m_AccumulatedStepMessages[0] = 0;
	m_AccumulatedStepMessages[1] = 0;
}

void ResourceBoundsPass::InsertBatched(SCommandBufferVersion &version, uint64_t key, const SDiagnosticMessageData & message, uint32_t count)
{
	// Merge if possible
	auto it = m_MessageLUT.find(key);
	if (it != m_MessageLUT.end())
	{
		auto&& msg = m_Messages[it->second];
		msg.m_MergedCount += count;
		m_AccumulatedStepMessages[msg.m_Error.m_ErrorType == VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA] += count;
		return;
	}

	VkGPUValidationMessageAVA msg{};
	msg.m_Type = VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA;
	msg.m_MergedCount = count;
	msg.m_Feature = VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS;
	msg.m_Error.m_ObjectInfo.m_Name = nullptr;
	msg.m_Error.m_ObjectInfo.m_Object = nullptr;
	msg.m_Error.m_UserMarkerCount = 0;

	// Import message
	auto imported = message.GetMessage<ResourceBoundsValidationMessage>();

	if (imported.m_ShaderSpanGUID != UINT32_MAX && m_State->m_DiagnosticRegistry->GetLocationRegistry()->GetExtractFromUID(imported.m_ShaderSpanGUID, &msg.m_Error.m_SourceExtract))
	{
		// Attempt to get associated binding
		ShaderLocationBinding binding{};
		if (m_State->m_DiagnosticRegistry->GetLocationRegistry()->GetBindingMapping(imported.m_ShaderSpanGUID, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS, &binding))
        {
		    STrackedWrite descriptor = version.GetDescriptorSet(binding.m_SetIndex).GetBinding(binding.m_BindingIndex);

		    // Get the object info
		    msg.m_Error.m_ObjectInfo = GetDescriptorObjectInfo(m_State, descriptor);
        }
	}

	switch (static_cast<ResourceBoundsValidationResourceType>(imported.m_ResourceType))
	{
	case ResourceBoundsValidationResourceType::eImage:
		msg.m_Error.m_Message = "Image address beyond view subresource range";
		msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_IMAGE_OVERFLOW_AVA;
		break;
	case ResourceBoundsValidationResourceType::eBuffer:
		msg.m_Error.m_Message = "Buffer address beyond view subresource range";
		msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA;
		break;
	}

	m_Messages.push_back(msg);
	m_MessageLUT.insert(std::make_pair(key, m_Messages.size() - 1));
	m_AccumulatedStepMessages[msg.m_Error.m_ErrorType == VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA] += count;
}
