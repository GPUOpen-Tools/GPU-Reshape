#include <SPIRV/InjectionPass.h>
#include <DiagnosticRegistry.h>
#include <CRC.h>
#include <StateTables.h>
#include <memory>

using namespace SPIRV;
using namespace spvtools;
using namespace spvtools::opt;

IRContext::Analysis InjectionPass::GetPreservedAnalyses()
{
	// States which the injection passes require
	return IRContext::kAnalysisDefUse | 
           IRContext::kAnalysisInstrToBlockMapping | 
		   IRContext::kAnalysisDecorations |
		   IRContext::kAnalysisCombinators | 
		   IRContext::kAnalysisNameMap |
		   IRContext::kAnalysisBuiltinVarId | 
		   IRContext::kAnalysisConstants;
}

spvtools::opt::BasicBlock* InjectionPass::SplitBasicBlock(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit, bool local)
{
	// Allocate label
	std::unique_ptr<Instruction> prelude_label(new Instruction(context(), SpvOpLabel, 0, TakeNextId(), {}));
	get_def_use_mgr()->AnalyzeInstDefUse(&*prelude_label);

	// Get local only lablel id
	uint32_t local_label_id  = TakeNextId();
	
	// Local only injection?
	if (local)
	{
		GetState()->m_UserLabelResultIDs.insert(local_label_id);
	}

	// Split the BB from iit on
	BasicBlock* new_block = block->SplitBasicBlock(context(), local_label_id, iit);

	// Push for analysis
	get_def_use_mgr()->AnalyzeInstDefUse(new_block->GetLabel().get());
	// cfg()->RegisterBlock(new_block);

	return new_block;
}

spvtools::opt::BasicBlock* InjectionPass::AllocBlock(spvtools::opt::BasicBlock* after, bool local)
{
	uint32_t label_id = TakeNextId();
	GetState()->m_UserLabelResultIDs.insert(label_id);

	// Local only injection?
	if (local)
	{
		GetState()->m_UserLabelResultIDs.insert(label_id);
	}

	// Allocate label
	std::unique_ptr<Instruction> label(new Instruction(context(), SpvOpLabel, 0, label_id, {}));

	// Allocate block
	std::unique_ptr<BasicBlock> block(new BasicBlock(std::move(label)));
	block->SetParent(after->GetParent());

	// Insert after specified block
	BasicBlock* moved_block = block->GetParent()->InsertBasicBlockAfter(std::move(block), after);

	// Push for analysis
	get_def_use_mgr()->AnalyzeInstDefUse(moved_block->GetLabel().get());
	// cfg()->RegisterBlock(moved_block);

	return moved_block;
}

std::unique_ptr<spvtools::opt::Instruction> InjectionPass::AllocInstr(SpvOp op, uint32_t ty_id, const spvtools::opt::Instruction::OperandList& in_operands)
{
	auto unique = std::make_unique<spvtools::opt::Instruction>(context(), op, ty_id, TakeNextId(), in_operands);
	MarkAsInjected(unique.get());
	return unique;
}

spvtools::opt::Instruction * InjectionPass::FindDeclarationType(uint32_t id)
{
	analysis::DefUseManager* def_mgr = get_def_use_mgr();
	
	Instruction* instr = def_mgr->GetDef(id);
	while (instr)
	{
		switch (instr->opcode())
		{
			default:
				return nullptr;
			case SpvOpLoad:
				instr = def_mgr->GetDef(instr->GetSingleWordOperand(2));
				break;
			case SpvOpVariable:
			{
				Instruction* type = def_mgr->GetDef(instr->GetSingleWordOperand(0));

				if (type->opcode() == SpvOpTypePointer)
					return def_mgr->GetDef(type->GetSingleWordOperand(2));
				else
					return type;
			}
		}
	}

	return nullptr;
}

spvtools::opt::Instruction * SPIRV::InjectionPass::FindDeclaration(uint32_t id)
{
	analysis::DefUseManager* def_mgr = get_def_use_mgr();

	Instruction* instr = def_mgr->GetDef(id);
	while (instr)
	{
		switch (instr->opcode())
		{
			default:
				return nullptr;
			case SpvOpLoad:
				instr = def_mgr->GetDef(instr->GetSingleWordOperand(2));
				break;
			case SpvOpVariable:
				return instr;
		}
	}

	return nullptr;
}

void InjectionPass::MarkAsInjected(const spvtools::opt::Instruction* id)
{
	GetState()->m_UserLocalInstructionIDs.insert(id);
}

bool InjectionPass::IsInjectedInstruction(const spvtools::opt::Instruction* id)
{
	return GetState()->m_UserLocalInstructionIDs.count(id);
}

opt::Pass::Status InjectionPass::Process()
{
	/* Hi there, I'm sure you're wondering what the below it. Fret not! It's inefficient code!
	   
	   Upon block injection there's no real guarentee which blocks have been modified and which blocks have been inserted.
	   So to avoid operating on potentially invalidated block iterators or missing blocks inserted before the current iterate we 
	   just loop back and "iterate again". A set is used to track which blocks have actually been visited.

	   ! If a second pass injection modifies a marked block then it's ignored. This doesn't happen in practise and I don't see why it would for the time being.
	*/

	std::set<uint32_t> blocks;

	bool visited;
	do
	{
		visited = false;

		for (auto f_it = get_module()->begin(); !visited && f_it < get_module()->end(); f_it++)
		{
			for (auto bb_it = f_it->begin(); bb_it < f_it->end(); bb_it++)
			{
				if (!blocks.count(bb_it->id()))
				{
					// Ignore all user blocks
					if (GetState()->m_UserLabelResultIDs.count(bb_it->GetLabel()->result_id()))
					{
						continue;
					}

					BasicBlock* basic_block = &*bb_it;
					Visit(basic_block);

					blocks.insert(basic_block->id());
					visited = true;
					break;
				}
			}
		}
	} while (visited);

	return Pass::Status::SuccessWithChange;
}

DescriptorState* InjectionPass::GetRegistryDescriptor(uint32_t set_id, uint16_t descriptor_uid)
{
	return &GetState()->m_RegistryDescriptorMergedLUT.at(descriptor_uid | (set_id << 16));
}

uint32_t SPIRV::InjectionPass::LoadPushConstant(spvtools::opt::InstructionBuilder& builder, uint16_t pc_uid)
{
	SPIRV::ShaderState* state = GetState();
	analysis::TypeManager* type_mgr = context()->get_type_mgr();

	// Get state description
	const SPIRV::PushConstantState& desc = GetState()->m_RegistryPushConstantLUT.at(pc_uid);

	// Get contained type pointer
	analysis::Pointer ptr_ty(type_mgr->GetType(desc.m_VarTypeID), SpvStorageClassPushConstant);
	uint32_t ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&ptr_ty));

	Instruction* ptr = builder.AddInstruction(AllocInstr(SpvOpAccessChain, ptr_ty_id,
	{
		Operand(SPV_OPERAND_TYPE_ID, { state->m_PushConstantVarID  }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(desc.m_ElementIndex) }), // Constant struct address
	}));

	return builder.AddLoad(desc.m_VarTypeID, ptr->result_id())->result_id();
}

uint32_t InjectionPass::CompositeStaticMessage(spvtools::opt::InstructionBuilder & builder, SDiagnosticMessageData data)
{
	return builder.GetUintConstantId(data.GetKey());
}

uint32_t InjectionPass::CompositeDynamicMessage(spvtools::opt::InstructionBuilder & builder, uint32_t type_id, uint32_t message_id)
{
	SPIRV::ShaderState* state = GetState();

	// Shift message left
	uint32_t mask_shl6 = builder.AddInstruction(AllocInstr(SpvOpShiftLeftLogical, state->m_DataBufferCounterType,
	{
		{ SPV_OPERAND_TYPE_ID, { message_id } },
		{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(6) } },
	}))->result_id();

	// Or it
	return builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, state->m_DataBufferCounterType,
	{
		{ SPV_OPERAND_TYPE_ID, { type_id } },
		{ SPV_OPERAND_TYPE_ID, { mask_shl6 } },
	}))->result_id();
}

uint32_t InjectionPass::PushMessages(spvtools::opt::InstructionBuilder & builder, uint32_t count)
{
	analysis::TypeManager* type_mgr = context()->get_type_mgr();
	SPIRV::ShaderState* state = GetState();

	// Get storage pointer types for access chaining
	analysis::Pointer counter_ptr_ty(type_mgr->GetType(state->m_DataBufferCounterType), SpvStorageClassStorageBuffer);
	uint32_t counter_ptr_ty_uid = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&counter_ptr_ty));

	Instruction* counter_ptr = builder.AddInstruction(AllocInstr(SpvOpAccessChain, counter_ptr_ty_uid,
	{
		Operand(SPV_OPERAND_TYPE_ID, { state->m_DataBufferVarID  }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) }), // Counter address
	}));

	// Note: OpAtomicIIncrement is using a deprecated capability
	Instruction* index = builder.AddInstruction(AllocInstr(SpvOpAtomicIAdd, state->m_DataBufferCounterType,
	{
		Operand(SPV_OPERAND_TYPE_ID, { counter_ptr->result_id() }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvScopeDevice) }), // ! Note that the scope is on the device !
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(1) })
	}));

	Instruction* limit_ptr = builder.AddInstruction(AllocInstr(SpvOpAccessChain, counter_ptr_ty_uid,
	{
		Operand(SPV_OPERAND_TYPE_ID, { state->m_DataBufferVarID  }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(1) }), // Limit address
	}));

	// Load limit
	Instruction* limit = builder.AddLoad(state->m_DataBufferCounterType, limit_ptr->result_id());

	// Current OOB behaviour is to min it, the alternative is an additional branch but that's getting expensive...
	index = builder.AddInstruction(AllocInstr(SpvOpExtInst, state->m_DataBufferCounterType,
	{
		Operand(SPV_OPERAND_TYPE_ID, { state->m_ExtendedGLSLStd450Set }),
		Operand(SPV_OPERAND_TYPE_LITERAL_INTEGER, { GLSLstd450UMin }),
		Operand(SPV_OPERAND_TYPE_ID, { index->result_id() }),
		Operand(SPV_OPERAND_TYPE_ID, { limit->result_id() }),
	}));

	return index->result_id();
}

void InjectionPass::ExportMessage(spvtools::opt::InstructionBuilder & builder, uint32_t id, uint32_t composite_id)
{
	analysis::TypeManager* type_mgr = context()->get_type_mgr();
	SPIRV::ShaderState* state = GetState();

	// Get storage pointer types for access chaining
	analysis::Pointer message_ptr_ty(type_mgr->GetType(state->m_DataMessageTypeID), SpvStorageClassStorageBuffer);
	uint32_t message_ptr_ty_uid = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&message_ptr_ty));

	Instruction* message_ptr = builder.AddInstruction(AllocInstr(SpvOpAccessChain, message_ptr_ty_uid,
	{
		Operand(SPV_OPERAND_TYPE_ID, { state->m_DataBufferVarID  }),
		Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(2) }), // Message buffer address
		Operand(SPV_OPERAND_TYPE_ID, { id }),							// Message address
	}));

	Instruction* message_composite = builder.AddCompositeConstruct(state->m_DataMessageTypeID, { composite_id });

	builder.AddStore(message_ptr->result_id(), message_composite->result_id());
}

void InjectionPass::ExportMessage(InstructionBuilder& builder, uint32_t composite_id)
{
	ExportMessage(builder, PushMessages(builder, 1), composite_id);
}

uint32_t InjectionPass::FindSourceExtractGUID(spvtools::opt::BasicBlock* block, spvtools::opt::BasicBlock::iterator iit)
{
	SPIRV::ShaderState* state   = GetState();
	analysis::DefUseManager* def_mgr = get_def_use_mgr();

	const VkGPUValidationCreateInfoAVA& create_info = GetState()->m_DeviceDispatchTable->m_CreateInfoAVA;

	// Name may be decorated
	auto&& fn_names = context()->GetNames(block->GetParent()->result_id());

	// Attempt to extract function name
	const char* function_name = nullptr;
	if (!fn_names.empty())
	{
		function_name = reinterpret_cast<const char*>(fn_names.begin()->second->GetOperand(1).words.data());
	}

	// Attempt to find candidate
	SourceCandidate candiate = FindCandidate(block, iit);
	if (!candiate.m_Instruction)
	{
		if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
		{
			create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] No line instruction is present, source extraction not possible (could be masked in parent CFG)");
		}

		if (state->m_SourceFileLUT.empty())
		{
			return UINT32_MAX;
		}

		return GetState()->m_DeviceState->m_DiagnosticRegistry->GetLocationRegistry()->RegisterFileExtract(state->m_SourceFileLUT.begin()->second, function_name);
	}

	// Get the OpString instruction
	Instruction* file_str_instr = def_mgr->GetDef(candiate.m_Instruction->GetSingleWordOperand(0));

	// To path
	auto path = reinterpret_cast<const char*>(file_str_instr->GetOperand(1).words.data());

	// Attempt to find mapped file
	auto file_uid_it = state->m_SourceFileLUT.find(path);
	if (file_uid_it == state->m_SourceFileLUT.end())
	{
		if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_ERROR))
		{
			create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_ERROR, __FILE__, __LINE__, "[SPIRV] The [file] operand of the line instruction is unmapped, skipping source extraction");
		}

		return UINT32_MAX;
	}

	// Attempt to register extract
	return GetState()->m_DeviceState->m_DiagnosticRegistry->GetLocationRegistry()->RegisterLineExtract(
		file_uid_it->second,
		function_name,
		candiate.m_Instruction->GetSingleWordOperand(1),
		candiate.m_Instruction->GetSingleWordOperand(2)
	);
}

InjectionPass::SourceCandidate InjectionPass::FindCandidate(spvtools::opt::BasicBlock * block, spvtools::opt::BasicBlock::iterator iit)
{
	SourceCandidate candidate;

	// Attempt inlined
	if ((candidate = FindInlinedSource(block, iit)).m_Instruction)
		return candidate;

	// Attempt callee
	if ((candidate = FindCalleeSource(block, iit)).m_Instruction)
		return candidate;

	return {};
}

InjectionPass::SourceCandidate InjectionPass::FindInlinedSource(spvtools::opt::BasicBlock * block, spvtools::opt::BasicBlock::iterator iit)
{
	// Search backward
	for (auto backwards = iit; backwards != block->begin(); --backwards)
	{
		if (!backwards->dbg_line_insts().empty())
		{
			// Relevant instruction is last
			return { &backwards->dbg_line_insts().back(), 0};
		}
	}

	// Search forward
	for (auto forward = iit; forward != block->end(); ++forward)
	{
		if (!forward->dbg_line_insts().empty())
		{
			// Relevant instruction is first
			return { &forward->dbg_line_insts().front(), 1 };
		}
	}

	return {};
}

InjectionPass::SourceCandidate InjectionPass::FindCalleeSource(spvtools::opt::BasicBlock * block, spvtools::opt::BasicBlock::iterator iit)
{
	// Search backwards for callee
	auto backwards = iit;
	while (backwards != block->begin())
	{
		--backwards;

		if (backwards->opcode() == SpvOpFunctionCall)
		{
			uint32_t fn_id = backwards->GetSingleWordOperand(2);

			opt::Function* fn = context()->GetFunction(fn_id);

			auto last_block_it = --fn->end();

			// Attempt to search this callee
			SourceCandidate candidate;
			if ((candidate = FindInlinedSource(&*last_block_it, last_block_it->end())).m_Instruction)
				return candidate;
		}
	}

	return {};
}

bool InjectionPass::GetDescriptorBinds(uint32_t id, uint32_t *set, uint32_t *binding)
{
    // Note: spirv-tools loves to waste memory, it's great
    std::vector<Instruction*> decorations = get_decoration_mgr()->GetDecorationsFor(id, false);

    // Dummy values
    *set = UINT32_MAX;
    *binding = UINT32_MAX;

    // Extract bindings
    for (auto&& decoration : decorations)
    {
        switch (decoration->GetSingleWordOperand(1))
        {
            case SpvDecorationDescriptorSet:
                *set = decoration->GetSingleWordOperand(2);
                break;
            case SpvDecorationBinding:
                *binding = decoration->GetSingleWordOperand(2);
                break;
        }
    }

    // Must both be found
    return *set != UINT32_MAX && *binding != UINT32_MAX;
}
