#include <Passes/Basic/RuntimeArrayBoundsPass.h>
#include <SPIRV/InjectionPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>
#include <DispatchTables.h>
#include <DiagnosticAllocator.h>
#include <CRC.h>

using namespace Passes;
using namespace Passes::Basic;

struct RuntimeArrayBoundsValidationMessage
{
	uint32_t m_ResourceType   :  1;
	uint32_t m_ShaderSpanGUID : kShaderLocationGUIDBits;
    uint32_t m_DeadBeef		  :  (kMessageBodyBits - kShaderLocationGUIDBits - 1);
};

class RuntimeArrayBoundsSPIRVPass : public SPIRV::InjectionPass
{
public:
	RuntimeArrayBoundsSPIRVPass(DiagnosticRegistry* registry, SPIRV::ShaderState* state, uint16_t error_uid, uint16_t descriptor_uid)
		: InjectionPass(state, "RuntimeArrayBoundsPass")
		, m_Registry(registry)
		, m_ErrorUID(error_uid)
		, m_DescriptorUID(descriptor_uid)
	{
	}

	bool IsBottomRuntimeArray(spvtools::opt::Instruction* instr, uint32_t* base_offset, uint32_t* set_id, uint32_t* binding_id)
	{
		using namespace spvtools;
		using namespace opt;

		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		switch (instr->opcode())
		{
			default:
				return false;
					
			case SpvOpAccessChain:
			{
				uint32_t base_id = instr->GetSingleWordOperand(2);

				*base_offset = instr->GetSingleWordOperand(3);

				// Runtime array must always be the base chained type
				return IsBottomRuntimeArray(get_def_use_mgr()->GetDef(base_id), base_offset, set_id, binding_id);
			}

			case SpvOpVariable:
			{
				analysis::Type* type = type_mgr->GetType(instr->GetSingleWordOperand(0));

				// Var types always pointer
				return type->AsPointer()->pointee_type()->kind() == analysis::Type::kRuntimeArray;
			}
		}
	}

	bool Visit(spvtools::opt::BasicBlock* block) override
	{
		using namespace spvtools;
		using namespace opt;

		analysis::DefUseManager* def_mgr = get_def_use_mgr();
		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Bool bool_ty;
		uint32_t	   bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

		for (auto iit = block->begin(); iit != block->end(); ++iit)
		{
			// Copied as it may be moved
			Instruction source_instr = *iit;

			switch (source_instr.opcode())
			{
				default: 
					break;

				/* Note that I could hook the access chain, but there are no guarentees that it isn't modified before the load! */
				case SpvOpLoad:
				{
					// Already instrumented?
					if (m_InstrumentedResults.count(&*iit) || IsInjectedInstruction(&*iit))
						continue;

					m_InstrumentedResults.insert(&*iit);

					uint32_t base_offset_id;

					// Get chain instruction
					Instruction* chain_instr = def_mgr->GetDef(source_instr.GetSingleWordOperand(2));

					// Is runtime array?
					uint32_t set_id;
					uint32_t binding_id;
					if (!IsBottomRuntimeArray(chain_instr, &base_offset_id, &set_id, &binding_id))
					{
						break;
					}

					// Attempt to find source extract
					uint32_t source_extract_guid = FindSourceExtractGUID(block, iit);
                    if (source_extract_guid != UINT32_MAX)
                    {
                        ShaderLocationBinding binding{};
                        binding.m_SetIndex = set_id;
                        binding.m_BindingIndex = binding_id;

                        // Register the mapping
                        m_Registry->GetLocationRegistry()->RegisterExtractBinding(source_extract_guid, VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS, binding);
                    }

					// Assumes uniform source instruction id
					uint32_t routed_result_id = TakeNextId();
					{
						iit->SetResultId(routed_result_id);
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
						Track(builder.AddBranch(post_block->GetLabel()->result_id()));
					}

					// The base block validates the offset, if OOB jumps to error otherwise offending
					uint32_t safe_base_value;
					{
						InstructionBuilder builder(context(), block);

						// TODO: Should we care about zero'ed descriptors?
						safe_base_value = builder.GetUintConstantId(0);

						// Get descriptor
						SPIRV::DescriptorState* descriptor = GetRegistryDescriptor(set_id, m_DescriptorUID);

						// Image fetch nonsense requirements
						spvtools::opt::analysis::Vector contained_vec(type_mgr->GetType(descriptor->m_ContainedTypeID), 4);
						uint32_t contained_vec_id = type_mgr->GetId(type_mgr->GetRegisteredType(&contained_vec));

						// Fetch bindless count
						Instruction* binding_fetch = builder.AddInstruction(AllocInstr(SpvOpImageFetch, contained_vec_id,
						{
							{ SPV_OPERAND_TYPE_ID, { builder.AddLoad(descriptor->m_VarTypeID, descriptor->m_VarID)->result_id() } }, // Image
							{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(binding_id) } }, // Coordinate
						}));

						// Get first element
						binding_fetch = Track(builder.AddCompositeExtract(descriptor->m_ContainedTypeID, binding_fetch->result_id(), { 0 }));
							
						// oob = address > size
						Instruction* state_vec = Track(builder.AddBinaryOp(bool_ty_id, SpvOpUGreaterThanEqual, base_offset_id, binding_fetch->result_id()));

						// oob ? error : offending
						Track(builder.AddConditionalBranch(state_vec->result_id(), error_block->GetLabel()->result_id(), offending_block->GetLabel()->result_id()));
					}

					// The error block writes error data and jumps to post
					{
						InstructionBuilder builder(context(), error_block);

						// Compose error message
						{
							RuntimeArrayBoundsValidationMessage message;
							message.m_ResourceType = 0;
							message.m_ShaderSpanGUID = source_extract_guid;
							message.m_DeadBeef = 0;

							ExportMessage(builder, CompositeStaticMessage(builder, SDiagnosticMessageData::Construct(m_ErrorUID, message)));
						}

						builder.AddBranch(post_block->GetLabel()->result_id());
					}

					// The post block needs to deduce the correct result value
					{
						// Create a copy of the original chained access and zero out the base
						std::unique_ptr<spvtools::opt::Instruction> access_copy(new spvtools::opt::Instruction(*chain_instr));
						{
							access_copy->SetResultId(TakeNextId());
							access_copy->SetOperand(3, { SPV_OPERAND_TYPE_ID, { safe_base_value } });
						}
						post_block->begin().InsertBefore(std::move(access_copy));

						// Select value based on previous control flow
						auto select = std::make_unique<spvtools::opt::Instruction>(
							context(), SpvOpPhi, source_instr.GetSingleWordOperand(0), source_instr.result_id(),
							Instruction::OperandList
							{
								Operand(SPV_OPERAND_TYPE_ID, { routed_result_id  }), Operand(SPV_OPERAND_TYPE_ID, { offending_block->id() }), // Offending block
								Operand(SPV_OPERAND_TYPE_ID, { access_copy->result_id() }), Operand(SPV_OPERAND_TYPE_ID, { error_block->id()     }), // Error block
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
	uint16_t			m_DescriptorUID;

	std::unordered_set<spvtools::opt::Instruction*> m_InstrumentedResults;
};

RuntimeArrayBoundsPass::RuntimeArrayBoundsPass(DeviceDispatchTable* table, DeviceStateTable* state) : m_Table(table), m_State(state)
{
	m_ErrorUID = m_State->m_DiagnosticRegistry->AllocateMessageUID();
	m_DescriptorUID = m_State->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_DescriptorStorageUID = m_State->m_DiagnosticRegistry->AllocateDescriptorStorageUID();

    m_State->m_DiagnosticRegistry->SetMessageHandler(m_ErrorUID, this);
}

void RuntimeArrayBoundsPass::Initialize(VkCommandBuffer cmd_buffer)
{
	// Dummy storage for when no DOI's are present
	CreateStorage(0, &m_DummyStorage);
}

void RuntimeArrayBoundsPass::Release()
{
	// Release unique storages
	for (RuntimeArrayBoundsDescriptorStorage* storage : m_StoragePool)
	{
		if (storage == m_DummyStorage)
			continue;

		m_Table->m_DestroyBufferView(m_Table->m_Device, storage->m_BufferView, nullptr);
		m_Table->m_DestroyBuffer(m_Table->m_Device, storage->m_Buffer, nullptr);
		m_State->m_DiagnosticAllocator->FreeDescriptorBinding(storage->m_Binding);
		delete storage;
	}

	// Release dummy storage
	m_Table->m_DestroyBufferView(m_Table->m_Device, m_DummyStorage->m_BufferView, nullptr);
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_DummyStorage->m_Buffer, nullptr);
    m_State->m_DiagnosticAllocator->FreeDescriptorBinding(m_DummyStorage->m_Binding);
	delete m_DummyStorage;
}

void RuntimeArrayBoundsPass::EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count)
{
	*count = 0;
}

void RuntimeArrayBoundsPass::EnumerateDescriptors(SDiagnosticDescriptorInfo* descriptors, uint32_t * count)
{
	*count = 1;

	// Write descriptors if requested
	if (descriptors)
	{
		auto&& info = descriptors[0] = {};
		info.m_UID = m_DescriptorUID;
		info.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		info.m_ElementFormat = VK_FORMAT_R32_UINT;
	}
}

VkResult RuntimeArrayBoundsPass::CreateStorage(uint32_t doi_count, RuntimeArrayBoundsDescriptorStorage **out)
{
	auto storage = new RuntimeArrayBoundsDescriptorStorage();
	storage->m_DOICount = doi_count;

	// Dummy value
	doi_count = std::max(doi_count, 1u);

	VkResult result;

	// Create buffer
	// Each DOI occupies 4 bytes
	VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	create_info.size = sizeof(uint32_t) * doi_count;
	create_info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &storage->m_Buffer)) != VK_SUCCESS)
	{
		return result;
	}

	// Get memory requirements
	VkMemoryRequirements requirements;
	m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, storage->m_Buffer, &requirements);

	// Create heap binding
	if ((result = m_State->m_DiagnosticAllocator->AllocateDescriptorBinding(requirements.alignment, requirements.size, &storage->m_Binding)) != VK_SUCCESS)
	{
		return result;
	}

	// Bind to said heap
	if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, storage->m_Buffer, storage->m_Binding.m_Heap->m_Memory.m_DeviceMemory, storage->m_Binding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
	{
		return result;
	}

	// Create appreproate view
	VkBufferViewCreateInfo view_info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
	view_info.buffer = storage->m_Buffer;
	view_info.format = VK_FORMAT_R32_UINT;
	view_info.offset = 0;
	view_info.range = VK_WHOLE_SIZE;
	if ((result = m_Table->m_CreateBufferView(m_Table->m_Device, &view_info, nullptr, &storage->m_BufferView)) != VK_SUCCESS)
	{
		return result;
	}

	*out = storage;
	return VK_SUCCESS;
}

void RuntimeArrayBoundsPass::CreateDescriptors(HDescriptorSet* set)
{
	// Count the number of descriptors of interest
	bool any_doi = false;
	for (const SDescriptor& descriptor : set->m_SetLayout->m_Descriptors)
	{
		if (descriptor.m_DescriptorCount > 1)
		{
			any_doi = true;
			break;
		}
	}

	// No DOI's?
	if (!any_doi)
	{
		set->m_Storage[m_DescriptorStorageUID] = m_DummyStorage;
		return;
	}

	// Get size
	uint32_t size = 0;
	for (const SDescriptor& descriptor : set->m_SetLayout->m_Descriptors)
	{
		size = std::max(size, descriptor.m_DstBinding + 1);
	}

	// Search through pool
	{
		std::lock_guard<std::mutex> guard(m_StorageLock);
		for (size_t i = 0; i < m_StoragePool.size(); i++)
		{
			if (m_StoragePool[i]->m_DOICount >= size)
			{
				set->m_Storage[m_DescriptorStorageUID] = m_StoragePool[i];
				m_StoragePool.erase(m_StoragePool.begin() + i);
				return;
			}
		}
	}

	// Create new one as none are available
	VkResult result = CreateStorage(size, reinterpret_cast<RuntimeArrayBoundsDescriptorStorage**>(&set->m_Storage[m_DescriptorStorageUID]));
	if (result != VK_SUCCESS)
	{
		return;
	}
}

void RuntimeArrayBoundsPass::DestroyDescriptors(HDescriptorSet* set)
{
	auto storage = static_cast<RuntimeArrayBoundsDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// May be dummy
	if (storage != m_DummyStorage) 
	{
		std::lock_guard<std::mutex> guard(m_StorageLock);
		m_StoragePool.push_back(storage);
	}
}

void RuntimeArrayBoundsPass::UpdateDescriptors(HDescriptorSet* set, bool update, bool push, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob)
{
	auto storage = static_cast<RuntimeArrayBoundsDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// Passthrough?
	if (update && storage->m_DOICount > 0)
	{
		auto data = static_cast<uint32_t*>(storage->m_Binding.m_MappedData);

		// Write top descriptor counts into storage
		for (uint32_t i = 0; i < top_count; i++)
		{
			data[top_descriptors[i].m_DstBinding] = top_descriptors[i].m_DescriptorCount;
		}
	}

	if (push)
	{
		// Write descriptor
		auto descriptor = reinterpret_cast<VkBufferView*>(&blob[diagnostic_descriptors[m_DescriptorUID].m_BlobOffset]);
		{
			*descriptor = storage->m_BufferView;
		}
	}
}

uint32_t RuntimeArrayBoundsPass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage)
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

void RuntimeArrayBoundsPass::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	optimizer->RegisterPass(SPIRV::CreatePassToken<RuntimeArrayBoundsSPIRVPass>(m_State->m_DiagnosticRegistry.get(), state, m_ErrorUID, m_DescriptorUID));
}

void RuntimeArrayBoundsPass::Step(VkGPUValidationReportAVA report)
{
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_DESCRIPTOR_OVERFLOW_AVA] += m_AccumulatedStepMessages;

	m_AccumulatedStepMessages = 0;
}

void RuntimeArrayBoundsPass::Report(VkGPUValidationReportAVA report)
{
	report->m_Messages.insert(report->m_Messages.end(), m_Messages.begin(), m_Messages.end());
}

void RuntimeArrayBoundsPass::Flush()
{
	m_Messages.clear();
	m_MessageLUT.clear();
	m_AccumulatedStepMessages = 0;
}

void RuntimeArrayBoundsPass::InsertBatched(SCommandBufferVersion &version, uint64_t key, const SDiagnosticMessageData & message, uint32_t count)
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
	msg.m_Feature = VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS;
	msg.m_Error.m_ObjectInfo.m_Name = nullptr;
	msg.m_Error.m_ObjectInfo.m_Object = nullptr;
	msg.m_Error.m_UserMarkerCount = 0;

	// Import message
	auto imported = message.GetMessage<RuntimeArrayBoundsValidationMessage>();

    if (imported.m_ShaderSpanGUID != UINT32_MAX && m_State->m_DiagnosticRegistry->GetLocationRegistry()->GetExtractFromUID(imported.m_ShaderSpanGUID, &msg.m_Error.m_SourceExtract))
    {
        // Attempt to get associated binding
        ShaderLocationBinding binding{};
        if (m_State->m_DiagnosticRegistry->GetLocationRegistry()->GetBindingMapping(imported.m_ShaderSpanGUID, VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS, &binding))
        {
            STrackedWrite descriptor = version.GetDescriptorSet(binding.m_SetIndex).GetBinding(binding.m_BindingIndex);

            // Get the object info
            msg.m_Error.m_ObjectInfo = GetDescriptorObjectInfo(m_State, descriptor);
        }
	}

	msg.m_Error.m_Message = "Runtime array index beyond array length";
	msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_DESCRIPTOR_OVERFLOW_AVA;

	m_Messages.push_back(msg);
	m_MessageLUT.insert(std::make_pair(key, m_Messages.size() - 1));
}
