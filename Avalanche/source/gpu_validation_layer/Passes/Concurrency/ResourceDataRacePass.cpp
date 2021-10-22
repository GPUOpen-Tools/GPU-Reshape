#include <Passes/Concurrency/ResourceDataRacePass.h>
#include <SPIRV/InjectionPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <CRC.h>

using namespace Passes;
using namespace Passes::Concurrency;

// Short lock uid export, useful for debugging lock mismatch issues
#ifndef RESOURCE_DATA_RACE_PASS_SHORTLOCKUID
#	define RESOURCE_DATA_RACE_PASS_SHORTLOCKUID 0
#endif

// The maximum number of resources that can be tracked
static constexpr uint64_t kMaxLockBufferResourceCount = 100'000;

struct ResourceDataRaceValidationMessage
{
	uint32_t m_ErrorType      :  1; // 25
	uint32_t m_ShaderSpanGUID : kShaderLocationGUIDBits; //  9
	uint32_t m_ShortLockKeyID :  (kShaderLocationGUIDBits - 1); //  0
};

class ResourceDataRaceSPIRVPass : public SPIRV::InjectionPass
{
public:
	ResourceDataRaceSPIRVPass(DiagnosticRegistry* registry, SPIRV::ShaderState* state, uint16_t error_uid, uint16_t global_lock_descriptor_uid, uint16_t metadata_descriptor_uid, uint16_t draw_id_pc_uid)
		: InjectionPass(state, "ResourceDataRacePass")
		, m_Registry(registry)
		, m_ErrorUID(error_uid)
		, m_GlobalLockDescriptorUID(global_lock_descriptor_uid)
		, m_MetadataDescriptorUID(metadata_descriptor_uid)
		, m_DrawIDPushConstantUID(draw_id_pc_uid)
	{
	}

	bool GetLockUID(spvtools::opt::InstructionBuilder& builder, spvtools::opt::Instruction* declaration, uint32_t* out_uid_id, uint32_t* out_set_id, ShaderLocationBinding* location_binding)
	{
		using namespace spvtools;
		using namespace spvtools::opt;

		SPIRV::ShaderState* state = GetState();

		const VkGPUValidationCreateInfoAVA& create_info = state->m_DeviceDispatchTable->m_CreateInfoAVA;

		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		// Note: spirv-tools loves to waste memory, it's great
		std::vector<Instruction*> decorations = get_decoration_mgr()->GetDecorationsFor(declaration->result_id(), false);

		uint32_t set_id = UINT32_MAX;
		uint32_t binding_id = UINT32_MAX;

		// Extract bindings
		for (auto&& decoration : decorations)
		{
			switch (decoration->GetSingleWordOperand(1))
			{
			case SpvDecorationDescriptorSet:
				set_id = decoration->GetSingleWordOperand(2);
				break;
			case SpvDecorationBinding:
				binding_id = decoration->GetSingleWordOperand(2);
				break;
			}
		}

		// Must have bindings
		if (set_id == UINT32_MAX || binding_id == UINT32_MAX)
		{
			if (create_info.m_LogCallback && (create_info.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
			{
				create_info.m_LogCallback(create_info.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, "[SPIRV] Failed to find image set and binding decorations, skipping instruction instrumentation");
			}

			return false;
		}

		// Copy binding information
        location_binding->m_SetIndex = set_id;
        location_binding->m_BindingIndex = binding_id;

		// Get metadata
		SPIRV::DescriptorState* metadata = GetRegistryDescriptor(set_id, m_MetadataDescriptorUID);

		// Uniform ptr
		spvtools::opt::analysis::Pointer ptr_ty(type_mgr->GetType(metadata->m_ContainedTypeID), SpvStorageClassUniform);
		uint32_t ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&ptr_ty));

		// Get ptr to lock uid
		Instruction* metadata_lock_ptr_uid = builder.AddAccessChain(ptr_ty_id, metadata->m_VarID,
		{
			builder.GetUintConstantId(0),		   // Runtime array
			builder.GetUintConstantId(binding_id), // Element
		});

		// Get first element
		*out_set_id = set_id;
		*out_uid_id = builder.AddLoad(metadata->m_ContainedTypeID, metadata_lock_ptr_uid->result_id())->result_id();

		// OK
		return true;
	}
	
	bool Visit(spvtools::opt::BasicBlock* block) override
	{
		using namespace spvtools;
		using namespace opt;

		analysis::DefUseManager* def_mgr = get_def_use_mgr();
		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Bool bool_ty;
		uint32_t	   bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

		analysis::Integer int_ty(32, false);
		uint32_t uint_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&int_ty));
		(void)uint_ty_id;

		for (auto iit = block->begin(); iit != block->end(); ++iit)
		{
			// Copied as it may be moved
			Instruction source_instr = *iit;

			// Get declaration
			spvtools::opt::Instruction* declaration = nullptr;
			switch (source_instr.opcode())
			{
			default:
				break;

				/* Read Operations */
				case SpvOpImageSampleImplicitLod:
				case SpvOpImageSampleExplicitLod:
				case SpvOpImageSampleDrefImplicitLod:
				case SpvOpImageSampleDrefExplicitLod:
				case SpvOpImageSampleProjImplicitLod:
				case SpvOpImageSampleProjExplicitLod:
				case SpvOpImageSampleProjDrefImplicitLod:
				case SpvOpImageSampleProjDrefExplicitLod:
				case SpvOpImageFetch:
				case SpvOpImageGather:
				case SpvOpImageDrefGather:
				case SpvOpImageRead:
				{
					Instruction* source = def_mgr->GetDef(source_instr.GetSingleWordOperand(2));
					if (source->opcode() == SpvOpSampledImage)
					{
						source = def_mgr->GetDef(source->GetSingleWordOperand(2));
					}

					declaration = FindDeclaration(source->result_id());
					break;
				}

				/* Write Operations */
				case SpvOpImageWrite:
				{
					declaration = FindDeclaration(source_instr.GetSingleWordOperand(0));
					break;
				}
			}

			// Any?
			if (!declaration)
				continue;

			// Already instrumented?
			if (m_InstrumentedResults.count(&*iit) || IsInjectedInstruction(&*iit))
				continue;

			m_InstrumentedResults.insert(&*iit);

			auto next = std::next(iit);

			// Shader binding
			ShaderLocationBinding binding{};

			// Attempt to get lock uid
			uint32_t lock_uid_id;
			uint32_t lock_set_id;
			{
				InstructionBuilder builder(context(), &*next);
				if (!GetLockUID(builder, declaration, &lock_uid_id, &lock_set_id, &binding))
				{
					break;
				}
			}

			// Attempt to find source extract
			uint32_t source_extract_guid = FindSourceExtractGUID(block, iit);
            if (source_extract_guid != UINT32_MAX)
			{
                // Register the mapping
                m_Registry->GetLocationRegistry()->RegisterExtractBinding(source_extract_guid, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE, binding);
            }

			// Write operation?
			bool is_write = (source_instr.opcode() == SpvOpImageWrite);

			// Create blocks
			// ... start ...
			//   BrCond Post Error
			// Error:
			//   WriteMessage
			//   Br Post
			// Post:
			//   <source>
			//   ...
			BasicBlock* post_block = SplitBasicBlock(block, next, false); /* Split the next instruction as OpSampledImage MUST be in the same block */
			BasicBlock* error_block = AllocBlock(block, true);

			// Pre block validates against lock uid
			uint32_t lock_ptr = 0;
			{
				InstructionBuilder builder(context(), block);

				// Get global lock
				SPIRV::DescriptorState* global_lock = GetRegistryDescriptor(lock_set_id, m_GlobalLockDescriptorUID);

				// As pointer
				analysis::Pointer texel_ptr_ty(type_mgr->GetType(global_lock->m_ContainedTypeID), SpvStorageClassImage);
				uint32_t texel_ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&texel_ptr_ty));

				// Get the address of our lock
				lock_ptr = builder.AddInstruction(AllocInstr(SpvOpImageTexelPointer, texel_ptr_ty_id,
				{
					Operand(SPV_OPERAND_TYPE_ID, { global_lock->m_VarID }),
					Operand(SPV_OPERAND_TYPE_ID, { lock_uid_id }),					// Address
					Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) }), // Sample
				}))->result_id();

				uint32_t is_thread_safe_id = 0;
				if (is_write)
				{
					/* Write Operation */

					// Load shared shader invocation id
					uint32_t shared_invocation_id = LoadPushConstant(builder, m_DrawIDPushConstantUID);

					// Compare exchange with current uid
					// If this shader is already locking it then the uid is consistent
					Instruction* previous_lock_value = builder.AddInstruction(AllocInstr(SpvOpAtomicCompareExchange, global_lock->m_ContainedTypeID,
					{
						Operand(SPV_OPERAND_TYPE_ID, { lock_ptr }),
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvScopeDevice) }),		  // ! Note that the scope is on the device !
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) }), // Equal   Semantics
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) }), // Unqueal Semantics
						Operand(SPV_OPERAND_TYPE_ID, { shared_invocation_id }),								  // Value
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) })						  // Comparator
					}));

					// Must be the shared value
					// v == 0 || v == shared_invo_id
					is_thread_safe_id = Track(builder.AddBinaryOp(
						bool_ty_id, SpvOpLogicalOr,
						builder.AddBinaryOp(bool_ty_id, SpvOpIEqual, previous_lock_value->result_id(), builder.GetUintConstantId(0))->result_id(),
						builder.AddBinaryOp(bool_ty_id, SpvOpIEqual, previous_lock_value->result_id(), shared_invocation_id)->result_id()
					))->result_id();
				}
				else
				{
					/* Read Operation */

					// Read the current lock value
					Instruction* lock_value = builder.AddInstruction(AllocInstr(SpvOpAtomicLoad, global_lock->m_ContainedTypeID,
					{
						Operand(SPV_OPERAND_TYPE_ID, { lock_ptr }),
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvScopeDevice) }), // ! Note that the scope is on the device !
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) })
					}));

					// Must be zero
					// i.e. no lock
					is_thread_safe_id = Track(builder.AddBinaryOp(bool_ty_id, SpvOpIEqual, lock_value->result_id(), builder.GetUintConstantId(0)))->result_id();
				}

				Track(builder.AddConditionalBranch(is_thread_safe_id, post_block->id(), error_block->id()));
			}

			// The error block writes error data and jumps to post
			{
				InstructionBuilder builder(context(), error_block);

				// Compose error message
				{
#if !RESOURCE_DATA_RACE_PASS_SHORTLOCKUID
					ResourceDataRaceValidationMessage message;
					message.m_ErrorType = static_cast<uint32_t>(is_write ? ResourceDataRaceValidationErrorType::eUnsafeWrite : ResourceDataRaceValidationErrorType::eUnsafeRead);
					message.m_ShaderSpanGUID = source_extract_guid;
					message.m_ShortLockKeyID = 0;

					ExportMessage(builder, CompositeStaticMessage(builder, SDiagnosticMessageData::Construct(m_ErrorUID, message)));
#else
					// Shift shader uid left
					uint32_t shader_uid_shl1 = builder.GetUintConstantId(source_extract_guid << 1);

					// Shift lock uid left
					uint32_t mask_shl17 = builder.AddInstruction(AllocInstr(SpvOpShiftLeftLogical, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { lock_uid_id } },
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(17) } },
					}))->result_id();

					// Composite message
					uint32_t message_id = builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
					{
						{ SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(static_cast<uint32_t>(is_write ? ResourceDataRaceValidationErrorType::eUnsafeWrite : ResourceDataRaceValidationErrorType::eUnsafeRead)) } },
						{ SPV_OPERAND_TYPE_ID, {
							builder.AddInstruction(AllocInstr(SpvOpBitwiseOr, uint_ty_id,
							{
								{ SPV_OPERAND_TYPE_ID, { shader_uid_shl1 } },
								{ SPV_OPERAND_TYPE_ID, { mask_shl17 } },
							}))->result_id()
						} },
					}))->result_id();

					// Export the message
					ExportMessage(builder, CompositeDynamicMessage(builder, builder.GetUintConstantId(m_ErrorUID), message_id));
#endif
				}

				builder.AddBranch(post_block->GetLabel()->result_id());
			}

			// The post block needs to unlock if a write
			if (is_write)
			{
				// Insert just after the IOI
				InstructionBuilder builder(context(), &*post_block->begin());

				// Unlock our lock
				auto unlock = std::make_unique<spvtools::opt::Instruction>(context(), SpvOpAtomicStore, 0, 0, spvtools::opt::Instruction::OperandList{
					Operand(SPV_OPERAND_TYPE_ID, { lock_ptr }),
					Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvScopeDevice) }), // ! Note that the scope is on the device !
					Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) }),
					Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(0) })
				});

				Track(unlock.get());
				builder.AddInstruction(std::move(unlock));
			}

			return true;
		}

		return true;
	}

private:
	DiagnosticRegistry* m_Registry;
	uint16_t			m_ErrorUID;
	uint16_t			m_GlobalLockDescriptorUID;
	uint16_t			m_MetadataDescriptorUID;
	uint16_t			m_DrawIDPushConstantUID;

	std::unordered_set<spvtools::opt::Instruction*> m_InstrumentedResults;
};

ResourceDataRacePass::ResourceDataRacePass(DeviceDispatchTable* table, DeviceStateTable* state)
	: m_Table(table), m_StateTable(state)
{
	m_ErrorUID = state->m_DiagnosticRegistry->AllocateMessageUID();
	m_GlobalLockDescriptorUID = state->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_MetadataDescriptorUID = state->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_DescriptorStorageUID = state->m_DiagnosticRegistry->AllocateDescriptorStorageUID();
	m_DrawIDPushConstantUID = state->m_DiagnosticRegistry->AllocatePushConstantUID();

	state->m_DiagnosticRegistry->SetMessageHandler(m_ErrorUID, this);
}

void ResourceDataRacePass::Initialize(VkCommandBuffer cmd_buffer)
{
	// Dummy storage for when no DOI's are present
	CreateStorage(0, &m_DummyStorage);

	// Create global lock buffer
	{
		VkResult result;

		// Create buffer
		// Each lock value occupies 4 bytes
		VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		create_info.size = sizeof(uint32_t) * (4 /* Stride Alignment Requirements*/) * kMaxLockBufferResourceCount;
		create_info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &m_GlobalLockBuffer)) != VK_SUCCESS)
		{
			return;
		}

		// Get memory requirements
		VkMemoryRequirements requirements;
		m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, m_GlobalLockBuffer, &requirements);

		// Create heap binding
		if ((result = m_StateTable->m_DiagnosticAllocator->AllocateDeviceBinding(requirements.alignment, requirements.size, &m_GlobalLockBinding)) != VK_SUCCESS)
		{
			return;
		}

		// Bind to said heap
		if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, m_GlobalLockBuffer, m_GlobalLockBinding.m_Heap->m_Memory.m_DeviceMemory, m_GlobalLockBinding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
		{
			return;
		}

		// Create appreproate view
		VkBufferViewCreateInfo view_info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
		view_info.buffer = m_GlobalLockBuffer;
		view_info.format = VK_FORMAT_R32_UINT;
		view_info.offset = 0;
		view_info.range = VK_WHOLE_SIZE;
		if ((result = m_Table->m_CreateBufferView(m_Table->m_Device, &view_info, nullptr, &m_GlobalLockBufferView)) != VK_SUCCESS)
		{
			return;
		}

		// Fill the initial lock values
		m_Table->m_CmdFillBuffer(cmd_buffer, m_GlobalLockBuffer, 0, create_info.size, 0);
	}
}

void ResourceDataRacePass::Release()
{
	// Release unique storages
	for (ResourceDataRaceDescriptorStorage* storage : m_StoragePool)
	{
		if (storage == m_DummyStorage)
			continue;

		m_Table->m_DestroyBuffer(m_Table->m_Device, storage->m_Buffer, nullptr);
		m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(storage->m_Binding);
		delete storage;
	}

	// Release dummy storage
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_DummyStorage->m_Buffer, nullptr);
	m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(m_DummyStorage->m_Binding);
	delete m_DummyStorage;

	// Release global lock buffer
	m_Table->m_DestroyBufferView(m_Table->m_Device, m_GlobalLockBufferView, nullptr);
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_GlobalLockBuffer, nullptr);
	m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(m_GlobalLockBinding);
}

void ResourceDataRacePass::EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count)
{
	*count = 0;
}

void ResourceDataRacePass::EnumerateDescriptors(SDiagnosticDescriptorInfo* descriptors, uint32_t * count)
{
	*count = 2;

	// Write descriptors if requested
	if (descriptors)
	{
		auto&& global_lock_descriptor = descriptors[0] = {};
		global_lock_descriptor.m_UID = m_GlobalLockDescriptorUID;
		global_lock_descriptor.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; // RW
		global_lock_descriptor.m_ElementFormat = VK_FORMAT_R32_UINT;

		auto&& metadata_descriptor = descriptors[1] = {};
		metadata_descriptor.m_UID = m_MetadataDescriptorUID;
		metadata_descriptor.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // RO
		metadata_descriptor.m_ElementFormat = VK_FORMAT_R32_UINT;
	}
}

void ResourceDataRacePass::EnumeratePushConstants(SDiagnosticPushConstantInfo* constants, uint32_t* count)
{
	*count = 1;

	if (constants)
	{
		auto&& constant = constants[0] = {};
		constant.m_UID = m_DrawIDPushConstantUID;
		constant.m_Format = VK_FORMAT_R32_UINT;
	}
}

size_t ResourceDataRacePass::UpdatePushConstants(VkCommandBuffer buffer, SPushConstantDescriptor* constants, uint8_t * data)
{
	// Get value ref
	auto&& value = *reinterpret_cast<uint32_t*>(&data[constants[m_DrawIDPushConstantUID].m_DataOffset]);
	{
		// Assign shared id
		// ! Cannot start at zero
		value = ++m_SharedIDCounter;
	}

	return sizeof(value);
}

VkResult ResourceDataRacePass::CreateStorage(uint32_t doi_count, ResourceDataRaceDescriptorStorage **out)
{
	auto storage = new ResourceDataRaceDescriptorStorage();
	storage->m_DOICount = doi_count;

	// Dummy value
	doi_count = std::max(doi_count, 1u);

	VkResult result;

	// Create buffer
	// Each DOI occupies 4 bytes
	VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size = (/*sizeof(uint32_t)*/ 16) * doi_count;
	create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &storage->m_Buffer)) != VK_SUCCESS)
	{
		return result;
	}

	// Get memory requirements
	VkMemoryRequirements requirements;
	m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, storage->m_Buffer, &requirements);

	// Create heap binding
	if ((result = m_StateTable->m_DiagnosticAllocator->AllocateDescriptorBinding(requirements.alignment, requirements.size, &storage->m_Binding)) != VK_SUCCESS)
	{
		return result;
	}

	// Bind to said heap
	if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, storage->m_Buffer, storage->m_Binding.m_Heap->m_Memory.m_DeviceMemory, storage->m_Binding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
	{
		return result;
	}

	*out = storage;
	return VK_SUCCESS;
}

void ResourceDataRacePass::CreateDescriptors(HDescriptorSet* set)
{
	// Count the number of descriptors of interest
	bool any_doi = false;
	for (const SDescriptor& descriptor : set->m_SetLayout->m_Descriptors)
	{
		switch (descriptor.m_DescriptorType)
		{
			default:
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
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
	VkResult result = CreateStorage(size, reinterpret_cast<ResourceDataRaceDescriptorStorage**>(&set->m_Storage[m_DescriptorStorageUID]));
	if (result != VK_SUCCESS)
	{
		return;
	}
}

void ResourceDataRacePass::DestroyDescriptors(HDescriptorSet* set)
{
	auto storage = static_cast<ResourceDataRaceDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// May be dummy
	if (storage != m_DummyStorage)
	{
		std::lock_guard<std::mutex> guard(m_StorageLock);
		m_StoragePool.push_back(storage);
	}
}

void ResourceDataRacePass::UpdateDescriptors(HDescriptorSet* set, bool update, bool push, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob)
{
	auto storage = static_cast<ResourceDataRaceDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// Passthrough?
	if (update && storage->m_DOICount > 0)
	{
		auto data = static_cast<uint8_t*>(storage->m_Binding.m_MappedData);

		// Write metadata lookup values
		for (uint32_t i = 0; i < top_count; i++)
		{
			auto&& descriptor = top_descriptors[i];

			// Get the key for locking
			void* key = nullptr;
			switch (descriptor.m_DescriptorType)
			{
				default:
					break;

				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					key = GetImageViewKey(reinterpret_cast<const VkDescriptorImageInfo*>(&blob[descriptor.m_BlobOffset])->imageView);
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					key = *reinterpret_cast<const VkBufferView*>(&blob[descriptor.m_BlobOffset]);
					break;

				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
					key = reinterpret_cast<const VkDescriptorBufferInfo*>(&blob[descriptor.m_BlobOffset])->buffer;
					break;
			}

			// Get offset
			uint32_t offset = diagnostic_descriptors[m_MetadataDescriptorUID].m_ArrayStride * descriptor.m_DstBinding;

			// Write lock data
			*reinterpret_cast<uint32_t*>(&data[offset]) = GetLockUID(key);
		}
	}

	if (push)
	{
		// Write global descriptor
		*reinterpret_cast<VkBufferView*>(&blob[diagnostic_descriptors[m_GlobalLockDescriptorUID].m_BlobOffset]) = m_GlobalLockBufferView;

		// Write metadata descriptor
		auto storage_info = reinterpret_cast<VkDescriptorBufferInfo*>(&blob[diagnostic_descriptors[m_MetadataDescriptorUID].m_BlobOffset]);
		storage_info->buffer = storage->m_Buffer;
		storage_info->offset = 0;
		storage_info->range = VK_WHOLE_SIZE;
	}
}

uint32_t ResourceDataRacePass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage)
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

void ResourceDataRacePass::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	optimizer->RegisterPass(SPIRV::CreatePassToken<ResourceDataRaceSPIRVPass>(m_StateTable->m_DiagnosticRegistry.get(), state, m_ErrorUID, m_GlobalLockDescriptorUID, m_MetadataDescriptorUID, m_DrawIDPushConstantUID));
}

void ResourceDataRacePass::Step(VkGPUValidationReportAVA report)
{
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA] += m_AccumulatedStepMessages;

	m_AccumulatedStepMessages = 0;
}

void ResourceDataRacePass::Report(VkGPUValidationReportAVA report)
{
	report->m_Messages.insert(report->m_Messages.end(), m_Messages.begin(), m_Messages.end());
}

void ResourceDataRacePass::Flush()
{
	m_Messages.clear();
	m_MessageLUT.clear();
	m_AccumulatedStepMessages = 0;
}

void ResourceDataRacePass::BeginRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo & info)
{
	// Mark all attached images as locked
	auto&& views = m_StateTable->m_ResourceFramebufferSources[info.framebuffer];
	for (auto view : views)
	{
		void* image_key = GetImageViewKey(view);

		// Using the command buffer as the lock value, "good enough"
		auto lock_data = static_cast<uint32_t>(reinterpret_cast<uint64_t>(cmd_buffer));

		// Write lock data
		m_Table->m_CmdUpdateBuffer(cmd_buffer, m_GlobalLockBuffer, GetLockUID(image_key) * sizeof(uint32_t), sizeof(uint32_t), &lock_data);
	}
}

void ResourceDataRacePass::EndRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo & info)
{
	// Mark all attached images as unlocked
	auto&& views = m_StateTable->m_ResourceFramebufferSources[info.framebuffer];
	for (auto view : views)
	{
		void* image_key = GetImageViewKey(view);

		// Note: 0 denotes unlocked
		uint32_t lock_data = 0;

		// Write free lock data
		m_Table->m_CmdUpdateBuffer(cmd_buffer, m_GlobalLockBuffer, GetLockUID(image_key) * sizeof(uint32_t), sizeof(uint32_t), &lock_data);
	}
}

uint32_t ResourceDataRacePass::GetLockUID(void * key)
{
	// Get a unique lock id
	auto&& lock_uid = m_LockOffsets[key];
	if (lock_uid == 0)
	{
		// Acquire uid
		lock_uid = static_cast<uint32_t>(m_LockOffsets.size());
	}

	return lock_uid;
}

void* ResourceDataRacePass::GetImageViewKey(const VkImageView & view)
{
	auto it = m_ImageViewKeys.find(view);
	if (it != m_ImageViewKeys.end())
		return it->second;

	const VkImageViewCreateInfo& info = m_StateTable->m_ResourceImageViewSources[view];

	uint64_t hash = reinterpret_cast<uint64_t>(info.image);

	CombineHash(hash, ComputeCRC64Buffer(info.subresourceRange));

	return (m_ImageViewKeys[view] = reinterpret_cast<void*>(hash));
}

void ResourceDataRacePass::InsertBatched(SCommandBufferVersion &version, uint64_t key, const SDiagnosticMessageData & message, uint32_t count)
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
	msg.m_Feature = VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE;
	msg.m_Error.m_ObjectInfo.m_Name = nullptr;
	msg.m_Error.m_ObjectInfo.m_Object = nullptr;
	msg.m_Error.m_UserMarkerCount = 0;

	// Import message
	auto imported = message.GetMessage<ResourceDataRaceValidationMessage>();

    if (imported.m_ShaderSpanGUID != UINT32_MAX && m_StateTable->m_DiagnosticRegistry->GetLocationRegistry()->GetExtractFromUID(imported.m_ShaderSpanGUID, &msg.m_Error.m_SourceExtract))
    {
        // Attempt to get associated binding
        ShaderLocationBinding binding{};
        if (m_StateTable->m_DiagnosticRegistry->GetLocationRegistry()->GetBindingMapping(imported.m_ShaderSpanGUID, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE, &binding))
        {
            STrackedWrite descriptor = version.GetDescriptorSet(binding.m_SetIndex).GetBinding(binding.m_BindingIndex);

            // Get the object info
            msg.m_Error.m_ObjectInfo = GetDescriptorObjectInfo(m_StateTable, descriptor);
        }
	}

	switch (static_cast<ResourceDataRaceValidationErrorType>(imported.m_ErrorType))
	{
		case ResourceDataRaceValidationErrorType::eUnsafeRead:
			msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA;
			msg.m_Error.m_Message = "Potential race condition detected, reading from a locked subresource";
			break;
		case ResourceDataRaceValidationErrorType::eUnsafeWrite:
			msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA;
			msg.m_Error.m_Message = "Potential race condition detected, writing to a locked subresource";
			break;
	}

	m_Messages.push_back(msg);
	m_MessageLUT.insert(std::make_pair(key, m_Messages.size() - 1));
}
