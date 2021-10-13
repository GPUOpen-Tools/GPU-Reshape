#include <Passes/DataResidency/ResourceInitializationPass.h>
#include <SPIRV/InjectionPass.h>
#include <ShaderLocationRegistry.h>
#include <Report.h>
#include <DispatchTables.h>
#include <StateTables.h>
#include <CRC.h>

// Short SRMASK export, useful for debugging SRMASK mismatch issues
#ifndef RESOURCE_INITIALZIATION_PASS_SHORTSRMASK
#	define RESOURCE_INITIALZIATION_PASS_SHORTSRMASK 0
#endif

// Shared Kernel Data
#include <gpu_validation/Passes/DataResidency/ResourceInitializationSharedData.h>

// Render Pass SRMask Kernel
static constexpr uint8_t kKernelSRMaskWrite[] = 
{
#	include <gpu_validation/Passes/DataResidency/ResourceInitializationSRMaskWrite.cb.h>
};

// Render Pass SRMask Kernel
static constexpr uint8_t kKernelSRMaskFree[] =
{
#	include <gpu_validation/Passes/DataResidency/ResourceInitializationSRMaskFree.cb.h>
};

using namespace Passes;
using namespace Passes::DataResidency;

// The maximum number of resource states that can be tracked
static constexpr uint64_t kMaxStateBufferResourceCount = 100'000;

// Uid lookup bit counts
static constexpr uint32_t kGlobalStateUIDBits   = 26;
static constexpr uint32_t kGlobalStateLayerBits = 6;  /* Assuming a maximum of 64 levels */

struct ResourceInitializationValidationMessage
{
#if RESOURCE_INITIALZIATION_PASS_SHORTSRMASK
    uint32_t m_ShaderSpanGUID : 16; //  10
    uint32_t m_AccessedSRMask :  5; //   0
    uint32_t m_WrittenSRMask  :  5; //   0
#else
    uint32_t m_ShaderSpanGUID : kShaderLocationGUIDBits;
    uint32_t m_DeadBeef       : (kMessageBodyBits - kShaderLocationGUIDBits);
#endif
};

struct ResourceInitializationExtendedMessage
{
	uint32_t m_ObjectRID : 26; //  0
};

class ResourceInitializationSPIRVPass : public SPIRV::InjectionPass
{
public:
	ResourceInitializationSPIRVPass(DiagnosticRegistry* registry, SPIRV::ShaderState* state, uint16_t error_uid, uint16_t global_lock_descriptor_uid, uint16_t metadata_rid_descriptor_uid, uint16_t metadata_rsmask_descriptor_uid)
		: InjectionPass(state, "ResourceInitializationPass")
		, m_Registry(registry)
		, m_ErrorUID(error_uid)
		, m_GlobalLockDescriptorUID(global_lock_descriptor_uid)
		, m_MetadataRIDDescriptorUID(metadata_rid_descriptor_uid)
		, m_MetadataSRMaskDescriptorUID(metadata_rsmask_descriptor_uid)
	{
	}

	bool GetLockData(spvtools::opt::InstructionBuilder& builder, spvtools::opt::Instruction* declaration, uint32_t* out_uid_id, uint32_t* out_srmask_id, uint32_t* out_set_id, ShaderLocationBinding* location_binding)
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

		// Get first element
		*out_set_id = set_id;

		// [RID]
		{
			// Get metadata
			SPIRV::DescriptorState* metadata = GetRegistryDescriptor(set_id, m_MetadataRIDDescriptorUID);

			// Uniform ptr
			spvtools::opt::analysis::Pointer ptr_ty(type_mgr->GetType(metadata->m_ContainedTypeID), SpvStorageClassUniform);
			uint32_t ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&ptr_ty));

			// Get ptr to lock uid
			Instruction* metadata_lock_ptr_uid = builder.AddAccessChain(ptr_ty_id, metadata->m_VarID,
			{
                    builder.GetUintConstantId(0),	   // Runtime array
				builder.GetUintConstantId(binding_id), // Element
			});

			// Load the id
			*out_uid_id = builder.AddLoad(metadata->m_ContainedTypeID, metadata_lock_ptr_uid->result_id())->result_id();
		}

		// [SRMASK]
		{
			// Get metadata
			SPIRV::DescriptorState* metadata = GetRegistryDescriptor(set_id, m_MetadataSRMaskDescriptorUID);

			// Uniform ptr
			spvtools::opt::analysis::Pointer ptr_ty(type_mgr->GetType(metadata->m_ContainedTypeID), SpvStorageClassUniform);
			uint32_t ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&ptr_ty));

			// Get ptr to lock uid
			Instruction* metadata_lock_ptr_uid = builder.AddAccessChain(ptr_ty_id, metadata->m_VarID,
			{
				builder.GetUintConstantId(0),	   // Runtime array
				builder.GetUintConstantId(binding_id), // Element
			});

			// Load the mask
			*out_srmask_id = builder.AddLoad(metadata->m_ContainedTypeID, metadata_lock_ptr_uid->result_id())->result_id();
		}

		// OK
		return true;
	}
	
	bool Visit(spvtools::opt::BasicBlock* block) override
	{
		using namespace spvtools;
		using namespace opt;

		SPIRV::ShaderState* state = GetState();

		analysis::DefUseManager* def_mgr = get_def_use_mgr();
		analysis::TypeManager* type_mgr = context()->get_type_mgr();

		analysis::Bool bool_ty;
		uint32_t	   bool_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&bool_ty));

		analysis::Integer int_ty(32, false);
		uint32_t uint_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&int_ty));

        analysis::Float fp32_ty(32);
        uint32_t fp32_ty_id = type_mgr->GetId(type_mgr->GetRegisteredType(&fp32_ty));

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

			// Base builder
			InstructionBuilder base_builder(context(), &*next);

            // Shader binding
            ShaderLocationBinding binding{};

			// Attempt to get lock uid
			uint32_t merged_state_id;
			uint32_t srmask_id;
			uint32_t lock_set_id;
			{
                if (!GetLockData(base_builder, declaration, &merged_state_id, &srmask_id, &lock_set_id, &binding))
				{
					break;
				}
			}

            // Get the state uid, mask out the layer bits
            uint32_t state_uid_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpBitwiseAnd, merged_state_id, base_builder.GetUintConstantId(~0u >> kGlobalStateLayerBits)))->result_id();

			// Attempt to find source extract
			uint32_t source_extract_guid = FindSourceExtractGUID(block, iit);
            if (source_extract_guid != UINT32_MAX)
            {
                // Register the mapping
                m_Registry->GetLocationRegistry()->RegisterExtractBinding(source_extract_guid, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION, binding);
            }

			// Write operation?
			bool is_write = (source_instr.opcode() == SpvOpImageWrite);

			// Explicit lod'ed operation?
			// Needs manual masking against external SRMASK
			switch (source_instr.opcode())
            {
                default:
                    /* To make the SA's shut up */
                    break;

                case SpvOpImageSampleExplicitLod:
                case SpvOpImageSampleDrefExplicitLod:
                case SpvOpImageSampleProjExplicitLod:
                case SpvOpImageSampleProjDrefExplicitLod:
                case SpvOpImageFetch:
                {
                    // Always lod by default
                    bool is_lod_fp = false;

                    // Determine image operand bits
                    uint32_t image_operands_index = 0;
                    switch (source_instr.opcode()) {
                        default:
                            /* To make the SA's shut up */
                            break;

                        case SpvOpImageSampleExplicitLod:
                        case SpvOpImageSampleProjExplicitLod:
                            image_operands_index = 4;
                            is_lod_fp = true;
                            break;
                        case SpvOpImageSampleProjDrefExplicitLod:
                        case SpvOpImageSampleDrefExplicitLod:
                            image_operands_index = 5;
                            is_lod_fp = true;
                            break;
                        case SpvOpImageFetch:
                            image_operands_index = 4;
                            break;
                    }

                    // Gradient operations assume whole subresource usage
                    if ((source_instr.GetSingleWordOperand(image_operands_index) & SpvImageOperandsLodMask) != SpvImageOperandsLodMask)
                    {
                        break;
                    }

                    // Has bias argument?
                    // The LOD argument is offset by the Bias argument as their appear in lowest bit order
                    bool has_bias = (source_instr.GetSingleWordOperand(image_operands_index) & SpvImageOperandsBiasMask) == SpvImageOperandsBiasMask;

                    // Load the lod index
                    uint32_t floating_lod_index_id = source_instr.GetSingleWordOperand(image_operands_index + 1 + has_bias);

                    // Explicit lod arguments may operate between subresource layers
                    uint32_t lower_lod_index_id = floating_lod_index_id;
                    uint32_t higher_lod_index_id = floating_lod_index_id;

                    // Floating point lod argument?
                    if (is_lod_fp)
                    {
                        // Get the lower bound
                        lower_lod_index_id = Track(base_builder.AddUnaryOp(uint_ty_id, SpvOpConvertFToU, Track(base_builder.AddInstruction(AllocInstr(SpvOpExtInst, fp32_ty_id,
                        {
                            Operand(SPV_OPERAND_TYPE_ID, { state->m_ExtendedGLSLStd450Set }),
                            Operand(SPV_OPERAND_TYPE_LITERAL_INTEGER, { GLSLstd450Floor }),
                            Operand(SPV_OPERAND_TYPE_ID, { floating_lod_index_id }),
                        })))->result_id()))->result_id();

                        // Get the upper bound
                        higher_lod_index_id = Track(base_builder.AddUnaryOp(uint_ty_id, SpvOpConvertFToU, Track(base_builder.AddInstruction(AllocInstr(SpvOpExtInst, fp32_ty_id,
                        {
                            Operand(SPV_OPERAND_TYPE_ID, { state->m_ExtendedGLSLStd450Set }),
                            Operand(SPV_OPERAND_TYPE_LITERAL_INTEGER, { GLSLstd450Ceil }),
                            Operand(SPV_OPERAND_TYPE_ID, { floating_lod_index_id }),
                        })))->result_id()))->result_id();
                    }

                    /*    LAYER
                            ->
                        x x x x x
                        x x x o x || LEVEL
                        x o x x x \/        <- O = LC * M, R = LC
                        x o x x x
                    */

                    // Get the layer count, mask out the uid bits
                    uint32_t layer_count_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpShiftRightArithmetic, merged_state_id, base_builder.GetUintConstantId(kGlobalStateUIDBits)))->result_id();

                    // The safe bit mask is acquired as such:
                    // (~0u >> (32 - LC)) << L * LC

                    // Get the inverse bit range
                    uint32_t lc_s32_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpISub, base_builder.GetUintConstantId(32), layer_count_id))->result_id();

                    // Get the bit mask of the first layer
                    uint32_t safe_base_lvl_mask_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpShiftRightLogical, base_builder.GetUintConstantId(~0u), lc_s32_id))->result_id();

                    // Shift the safe (lower) layer mask into the correct offset
                    uint32_t lower_safe_mask_id = Track(base_builder.AddBinaryOp(
                            uint_ty_id, SpvOpShiftLeftLogical,
                            safe_base_lvl_mask_id,
                            Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpIMul, lower_lod_index_id, layer_count_id))->result_id()
                    ))->result_id();

                    // Account for higher mask if needed
                    uint32_t safe_mask_id = lower_safe_mask_id;
                    if (higher_lod_index_id != lower_lod_index_id)
                    {
                        // Shift the safe higher layer mask into the correct offset
                        uint32_t higher_safe_mask_id = Track(base_builder.AddBinaryOp(
                                uint_ty_id, SpvOpShiftLeftLogical,
                                safe_base_lvl_mask_id,
                                Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpIMul, higher_lod_index_id, layer_count_id))->result_id()
                        ))->result_id();

                        // Combine lower and higher mask
                        safe_mask_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpBitwiseOr, lower_safe_mask_id, higher_safe_mask_id))->result_id();
                    }

                    // Mask out the parent mask with the
                    srmask_id = Track(base_builder.AddBinaryOp(uint_ty_id, SpvOpBitwiseAnd, srmask_id, safe_mask_id))->result_id();
                    break;
                }
            }

			// Get global lock
			SPIRV::DescriptorState* global_lock = GetRegistryDescriptor(lock_set_id, m_GlobalLockDescriptorUID);

			// Pre block validates against lock uid
			uint32_t state_ptr = 0;
			{
				// As pointer
				analysis::Pointer texel_ptr_ty(type_mgr->GetType(global_lock->m_ContainedTypeID), SpvStorageClassImage);
				uint32_t texel_ptr_ty_id = type_mgr->GetTypeInstruction(type_mgr->GetRegisteredType(&texel_ptr_ty));

				// Get the address of our state
				state_ptr = base_builder.AddInstruction(AllocInstr(SpvOpImageTexelPointer, texel_ptr_ty_id,
				{
					Operand(SPV_OPERAND_TYPE_ID, { global_lock->m_VarID }),
					Operand(SPV_OPERAND_TYPE_ID, { state_uid_id }),					// Address
					Operand(SPV_OPERAND_TYPE_ID, { base_builder.GetUintConstantId(0) }), // Sample
				}))->result_id();
			}

			// Write operation do not require error handling
			if (is_write)
			{
				// Perform or with current subresource range
				base_builder.AddInstruction(AllocInstr(SpvOpAtomicOr, global_lock->m_ContainedTypeID,
				{
					Operand(SPV_OPERAND_TYPE_ID, { state_ptr }),
					Operand(SPV_OPERAND_TYPE_ID, { base_builder.GetUintConstantId(SpvScopeDevice) }),			  // ! Note that the scope is on the device !
					Operand(SPV_OPERAND_TYPE_ID, { base_builder.GetUintConstantId(SpvMemoryAccessMaskNone) }),   // Semantics
					Operand(SPV_OPERAND_TYPE_ID, { srmask_id })													      // Value
				}));

				// Set the new iteration point
                // Note: Decrement as will be incremented upon next step
				--(iit = base_builder.GetInsertPoint());
			}
			else
			{
				/* Read Operation */

				// Create blocks
				// ... start ...
				//   BrCond Post Error
				// Error:
				//   WriteMessage
				//   Br Post
				// Post:
				//   <source>
				//   ...
				BasicBlock* post_block = SplitBasicBlock(block, base_builder.GetInsertPoint(), false); /* Split just before the current insertion point */
				BasicBlock* error_block = AllocBlock(block, true);

				// Pre-block
				uint32_t loaded_sr_mask_id;
				{
					// Base builder
					InstructionBuilder builder(context(), block);

					// Read the current state mask
                    loaded_sr_mask_id = builder.AddInstruction(AllocInstr(SpvOpAtomicLoad, global_lock->m_ContainedTypeID,
					{
						Operand(SPV_OPERAND_TYPE_ID, { state_ptr }),
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvScopeDevice) }),		  // ! Note that the scope is on the device !
						Operand(SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(SpvMemoryAccessMaskNone) })
					}))->result_id();

					// Ensure that the current mask can at least accomodate for the expected mask!
					// { G[RID] & SRMask == SRMask }
					Instruction* can_accomodate = Track(builder.AddBinaryOp(
						bool_ty_id, SpvOpIEqual,
						builder.AddBinaryOp(uint_ty_id, SpvOpBitwiseAnd, loaded_sr_mask_id, srmask_id)->result_id(),
						srmask_id
					));

					// Any deviating bit indicates a potentially uninitialized read!
					Track(builder.AddConditionalBranch(can_accomodate->result_id(), post_block->id(), error_block->id()));
				}

				// The error block writes error data and jumps to post
				{
					InstructionBuilder builder(context(), error_block);

					// Compose error message
					{
#if RESOURCE_INITIALZIATION_PASS_SHORTSRMASK
                        // Shift accessed sr mask
                        uint32_t accessed_shl16 = builder.AddInstruction(AllocInstr(SpvOpShiftLeftLogical, uint_ty_id,
                        {
                            { SPV_OPERAND_TYPE_ID, { srmask_id } },
                            { SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(16) } },
                        }))->result_id();

                        // Shift written sr mask
                        uint32_t written_shl16p5 = builder.AddInstruction(AllocInstr(SpvOpShiftLeftLogical, uint_ty_id,
                        {
                                { SPV_OPERAND_TYPE_ID, { loaded_sr_mask_id } },
                                { SPV_OPERAND_TYPE_ID, { builder.GetUintConstantId(16 + 5 /* accessed */) } },
                        }))->result_id();

                        // Composite message
                        uint32_t message_id = builder.AddBinaryOp(
                                uint_ty_id, SpvOpBitwiseOr,
                                builder.GetUintConstantId(source_extract_guid),
                                builder.AddBinaryOp(uint_ty_id, SpvOpBitwiseOr, accessed_shl16, written_shl16p5)->result_id()
                        )->result_id();

                        // Export the message
                        ExportMessage(builder, CompositeDynamicMessage(builder, builder.GetUintConstantId(m_ErrorUID), message_id));
#else
						ResourceInitializationValidationMessage message;
						message.m_ShaderSpanGUID = source_extract_guid;
						message.m_DeadBeef = 0;

						ExportMessage(builder, CompositeStaticMessage(builder, SDiagnosticMessageData::Construct(m_ErrorUID, message)));
#endif
					}

					builder.AddBranch(post_block->GetLabel()->result_id());
				}

                return true;
			}
		}

		return true;
	}

private:
	DiagnosticRegistry* m_Registry;
	uint16_t			m_ErrorUID;
	uint16_t			m_GlobalLockDescriptorUID;
	uint16_t			m_MetadataRIDDescriptorUID;
	uint16_t			m_MetadataSRMaskDescriptorUID;

	std::unordered_set<spvtools::opt::Instruction*> m_InstrumentedResults;
};

ResourceInitializationPass::ResourceInitializationPass(DeviceDispatchTable* table, DeviceStateTable* state)
	: m_Table(table), m_StateTable(state)
{
	m_ErrorUID = state->m_DiagnosticRegistry->AllocateMessageUID();
	m_GlobalStateDescriptorUID = state->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_MetadataRIDDescriptorUID = state->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_MetadataSRMaskDescriptorUID = state->m_DiagnosticRegistry->AllocateDescriptorUID();
	m_DescriptorStorageUID = state->m_DiagnosticRegistry->AllocateDescriptorStorageUID();

	state->m_DiagnosticRegistry->SetMessageHandler(m_ErrorUID, this);
}

void ResourceInitializationPass::Initialize(VkCommandBuffer cmd_buffer)
{
	// Dummy storage for when no DOI's are present
	CreateStorage(0, &m_DummyStorage);

	// Initialize mirror state
    m_GlobalStateMirror.resize(kMaxStateBufferResourceCount);

	// Create global lock buffer
	{
		VkResult result;

		// Create buffer
		// Each lock value occupies 4 bytes
		VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		create_info.size = sizeof(uint32_t) * (4 /* Stride Alignment Requirements*/) * kMaxStateBufferResourceCount;
		create_info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &m_GlobalStateBuffer)) != VK_SUCCESS)
		{
			return;
		}

		// Get memory requirements
		VkMemoryRequirements requirements;
		m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, m_GlobalStateBuffer, &requirements);

		// Create heap binding
		if ((result = m_StateTable->m_DiagnosticAllocator->AllocateDeviceBinding(requirements.alignment, requirements.size, &m_GlobalStateBinding)) != VK_SUCCESS)
		{
			return;
		}

		// Bind to said heap
		if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, m_GlobalStateBuffer, m_GlobalStateBinding.m_Heap->m_Memory.m_DeviceMemory, m_GlobalStateBinding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
		{
			return;
		}

		// Create appropriate view
		VkBufferViewCreateInfo view_info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
		view_info.buffer = m_GlobalStateBuffer;
		view_info.format = VK_FORMAT_R32_UINT;
		view_info.offset = 0;
		view_info.range = VK_WHOLE_SIZE;
		if ((result = m_Table->m_CreateBufferView(m_Table->m_Device, &view_info, nullptr, &m_GlobalStateBufferView)) != VK_SUCCESS)
		{
			return;
		}

		// Fill the initial lock values
		m_Table->m_CmdFillBuffer(cmd_buffer, m_GlobalStateBuffer, 0, create_info.size, 0);
	}

	// Create SRMask write kernel
	{
		VkDescriptorType descriptor_types[] =
		{
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
		};

		ComputeKernelInfo info;
		info.Kernel(kKernelSRMaskWrite);
		info.DescriptorTypes(descriptor_types);
		info.m_PCByteSpan = sizeof(ResourceInitializationSRMaskWriteData);
		m_KernelSRMaskWrite.Initialize(m_Table->m_Device, info);

		// Write descriptors
		ComputeKernelDescriptor descriptor;
		descriptor.m_TexelBufferInfo = m_GlobalStateBufferView;
		m_KernelSRMaskWrite.UpdateDescriptors(&descriptor);
	}

    // Create SRMask free kernel
    {
        VkDescriptorType descriptor_types[] =
        {
                VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        };

        ComputeKernelInfo info;
        info.Kernel(kKernelSRMaskFree);
        info.DescriptorTypes(descriptor_types);
        info.m_PCByteSpan = sizeof(ResourceInitializationSRMaskFreeData);
        m_KernelSRMaskFree.Initialize(m_Table->m_Device, info);

        // Write descriptors
        ComputeKernelDescriptor descriptor;
        descriptor.m_TexelBufferInfo = m_GlobalStateBufferView;
        m_KernelSRMaskFree.UpdateDescriptors(&descriptor);
    }
}

void ResourceInitializationPass::Release()
{
	// Release unique storages
	for (ResourceInitializationDescriptorStorage* storage : m_StoragePool)
	{
		if (storage == m_DummyStorage)
			continue;

		m_Table->m_DestroyBuffer(m_Table->m_Device, storage->m_RIDBuffer, nullptr);
		m_Table->m_DestroyBuffer(m_Table->m_Device, storage->m_RSMaskBuffer, nullptr);
		m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(storage->m_RIDBinding);
		m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(storage->m_RSMaskBinding);
		delete storage;
	}

	// Release dummy storage
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_DummyStorage->m_RIDBuffer, nullptr);
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_DummyStorage->m_RSMaskBuffer, nullptr);
	m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(m_DummyStorage->m_RIDBinding);
	m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(m_DummyStorage->m_RSMaskBinding);
	delete m_DummyStorage;

	// Release global lock buffer
	m_Table->m_DestroyBufferView(m_Table->m_Device, m_GlobalStateBufferView, nullptr);
	m_Table->m_DestroyBuffer(m_Table->m_Device, m_GlobalStateBuffer, nullptr);
	m_StateTable->m_DiagnosticAllocator->FreeDescriptorBinding(m_GlobalStateBinding);
}

void ResourceInitializationPass::EnumerateStorage(SDiagnosticStorageInfo* storage, uint32_t* count)
{
	*count = 0;
}

void ResourceInitializationPass::EnumerateDescriptors(SDiagnosticDescriptorInfo* descriptors, uint32_t * count)
{
	*count = 3;

	// Write descriptors if requested
	if (descriptors)
	{
		auto&& global_lock_descriptor = descriptors[0] = {};
		global_lock_descriptor.m_UID = m_GlobalStateDescriptorUID;
		global_lock_descriptor.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; // RW
		global_lock_descriptor.m_ElementFormat = VK_FORMAT_R32_UINT;

		auto&& metadata_rid_descriptor = descriptors[1] = {};
		metadata_rid_descriptor.m_UID = m_MetadataRIDDescriptorUID;
		metadata_rid_descriptor.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // RO
		metadata_rid_descriptor.m_ElementFormat = VK_FORMAT_R32_UINT;

		auto&& metadata_srmask_descriptor = descriptors[2] = {};
		metadata_srmask_descriptor.m_UID = m_MetadataSRMaskDescriptorUID;
		metadata_srmask_descriptor.m_DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // RO
		metadata_srmask_descriptor.m_ElementFormat = VK_FORMAT_R32_UINT;
	}
}

VkResult ResourceInitializationPass::CreateStorage(uint32_t doi_count, ResourceInitializationDescriptorStorage **out)
{
	auto storage = new ResourceInitializationDescriptorStorage();
	storage->m_DOICount = doi_count;

	// Dummy value
	doi_count = std::max(doi_count, 1u);

	VkResult result;

	// [RID]
	{
		// Create buffer
		// Each DOI occupies 4 bytes
		VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		create_info.size = (/*sizeof(uint32_t)*/ 16) * doi_count;
		create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &storage->m_RIDBuffer)) != VK_SUCCESS)
		{
			return result;
		}

		// Get memory requirements
		VkMemoryRequirements requirements;
		m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, storage->m_RIDBuffer, &requirements);

		// Create heap binding
		if ((result = m_StateTable->m_DiagnosticAllocator->AllocateDescriptorBinding(requirements.alignment, requirements.size, &storage->m_RIDBinding)) != VK_SUCCESS)
		{
			return result;
		}

		// Bind to said heap
		if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, storage->m_RIDBuffer, storage->m_RIDBinding.m_Heap->m_Memory.m_DeviceMemory, storage->m_RIDBinding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
		{
			return result;
		}
	}

	// [RSMASK]
	{
		// Create buffer
		// Each DOI occupies 4 bytes
		VkBufferCreateInfo create_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		create_info.size = (/*sizeof(uint32_t)*/ 16) * doi_count;
		create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if ((result = m_Table->m_CreateBuffer(m_Table->m_Device, &create_info, nullptr, &storage->m_RSMaskBuffer)) != VK_SUCCESS)
		{
			return result;
		}

		// Get memory requirements
		VkMemoryRequirements requirements;
		m_Table->m_GetBufferMemoryRequirements(m_Table->m_Device, storage->m_RSMaskBuffer, &requirements);

		// Create heap binding
		if ((result = m_StateTable->m_DiagnosticAllocator->AllocateDescriptorBinding(requirements.alignment, requirements.size, &storage->m_RSMaskBinding)) != VK_SUCCESS)
		{
			return result;
		}

		// Bind to said heap
		if ((result = m_Table->m_BindBufferMemory(m_Table->m_Device, storage->m_RSMaskBuffer, storage->m_RSMaskBinding.m_Heap->m_Memory.m_DeviceMemory, storage->m_RSMaskBinding.m_AllocationIt->m_Offset)) != VK_SUCCESS)
		{
			return result;
		}
	}

	*out = storage;
	return VK_SUCCESS;
}

void ResourceInitializationPass::CreateDescriptors(HDescriptorSet* set)
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
	VkResult result = CreateStorage(size, reinterpret_cast<ResourceInitializationDescriptorStorage**>(&set->m_Storage[m_DescriptorStorageUID]));
	if (result != VK_SUCCESS)
	{
		return;
	}
}

void ResourceInitializationPass::DestroyDescriptors(HDescriptorSet* set)
{
	auto storage = static_cast<ResourceInitializationDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// May be dummy
	if (storage != m_DummyStorage)
	{
		std::lock_guard<std::mutex> guard(m_StorageLock);
		m_StoragePool.push_back(storage);
	}
}

void ResourceInitializationPass::UpdateDescriptors(HDescriptorSet* set, bool update, bool push, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob)
{
	auto storage = static_cast<ResourceInitializationDescriptorStorage*>(set->m_Storage[m_DescriptorStorageUID]);

	// Passthrough?
	if (update && storage->m_DOICount > 0)
	{
		// Write metadata lookup values
		for (uint32_t i = 0; i < top_count; i++)
		{
			auto&& descriptor = top_descriptors[i];

			// Get the key, mask and level count for locking
			void* key = nullptr;
			uint32_t mask = 0;
			uint32_t layer_count = 0;
			switch (descriptor.m_DescriptorType)
			{
				default:
					break;

				case VK_DESCRIPTOR_TYPE_SAMPLER:
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				{
					auto view = reinterpret_cast<const VkDescriptorImageInfo*>(&blob[descriptor.m_BlobOffset])->imageView;

					key = GetImageViewKey(view);
					mask = GetImageViewSRMask(view);
                    layer_count = m_StateTable->m_ResourceImageSources[m_StateTable->m_ResourceImageViewSources[view].image].arrayLayers;
					break;
				}

				// Note: Buffers do not use subresource state tracking
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				{
				    // The underlying buffer is used as the key!
					key = m_StateTable->m_ResourceBufferViewSources[*reinterpret_cast<const VkBufferView*>(&blob[descriptor.m_BlobOffset])].buffer;
					mask = 1;
                    layer_count = 1;
					break;
				}

				// Note: Buffers do not use subresource state tracking
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				{
					key = reinterpret_cast<const VkDescriptorBufferInfo*>(&blob[descriptor.m_BlobOffset])->buffer;
					mask = 1;
                    layer_count = 1;
					break;
				}
			}

			// Get offsets
			uint32_t rid_offset = diagnostic_descriptors[m_MetadataRIDDescriptorUID].m_ArrayStride * descriptor.m_DstBinding;
			uint32_t srmask_offset = diagnostic_descriptors[m_MetadataSRMaskDescriptorUID].m_ArrayStride * descriptor.m_DstBinding;

			// Merge state
			uint32_t merged_state = GetLockUID(key) | (layer_count << kGlobalStateUIDBits);

			// Write RID & RSMASK
			*reinterpret_cast<uint32_t*>(&static_cast<uint8_t*>(storage->m_RIDBinding.m_MappedData)[rid_offset]) = merged_state;
			*reinterpret_cast<uint32_t*>(&static_cast<uint8_t*>(storage->m_RSMaskBinding.m_MappedData)[srmask_offset]) = mask;
		}
	}

	if (push)
	{
		// Write global descriptor
		*reinterpret_cast<VkBufferView*>(&blob[diagnostic_descriptors[m_GlobalStateDescriptorUID].m_BlobOffset]) = m_GlobalStateBufferView;

		// Write metadata RID descriptor
		{
			auto storage_info = reinterpret_cast<VkDescriptorBufferInfo*>(&blob[diagnostic_descriptors[m_MetadataRIDDescriptorUID].m_BlobOffset]);
			storage_info->buffer = storage->m_RIDBuffer;
			storage_info->offset = 0;
			storage_info->range = VK_WHOLE_SIZE;
		}

		// Write metadata RID descriptor
		{
			auto storage_info = reinterpret_cast<VkDescriptorBufferInfo*>(&blob[diagnostic_descriptors[m_MetadataSRMaskDescriptorUID].m_BlobOffset]);
			storage_info->buffer = storage->m_RSMaskBuffer;
			storage_info->offset = 0;
			storage_info->range = VK_WHOLE_SIZE;
		}
	}
}

uint32_t ResourceInitializationPass::Handle(SStringCache *message_cache, SCommandBufferVersion &version, const SDiagnosticMessageData *messages, uint32_t count, void *const *storage)
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
				InsertBatched(version, message_cache, batch_key, messages[i - 1], batch_count);
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
		InsertBatched(version, message_cache, messages[count - 1].GetKey(), messages[count - 1], batch_count);
		handled += batch_count;
	}

	return handled;
}

void ResourceInitializationPass::Register(SPIRV::ShaderState* state, spvtools::Optimizer* optimizer)
{
	optimizer->RegisterPass(SPIRV::CreatePassToken<ResourceInitializationSPIRVPass>(m_StateTable->m_DiagnosticRegistry.get(), state, m_ErrorUID, m_GlobalStateDescriptorUID, m_MetadataRIDDescriptorUID, m_MetadataSRMaskDescriptorUID));
}

void ResourceInitializationPass::Step(VkGPUValidationReportAVA report)
{
	report->m_Steps.back().m_ErrorCounts[VK_GPU_VALIDATION_ERROR_TYPE_SUBRESOURCE_UNINITIALIZED] += m_AccumulatedStepMessages;

	m_AccumulatedStepMessages = 0;
}

void ResourceInitializationPass::Report(VkGPUValidationReportAVA report)
{
	report->m_Messages.insert(report->m_Messages.end(), m_Messages.begin(), m_Messages.end());
}

void ResourceInitializationPass::Flush()
{
	m_Messages.clear();
	m_MessageLUT.clear();
	m_AccumulatedStepMessages = 0;
}

void ResourceInitializationPass::BeginRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo & info)
{
	// Mark all attached images as locked
	auto&& views = m_StateTable->m_ResourceFramebufferSources[info.framebuffer];
	for (auto view : views)
	{
		ResourceInitializationSRMaskWriteData data{};
		data.m_RID = GetLockUID(GetImageViewKey(view));
		data.m_SRMask = GetImageViewSRMask(view);

		if ((m_GlobalStateMirror[data.m_RID] & data.m_SRMask) == data.m_SRMask)
		    continue;

        m_GlobalStateMirror[data.m_RID] |= data.m_SRMask;
		m_KernelSRMaskWrite.Dispatch(cmd_buffer, data);
	}
}

void ResourceInitializationPass::EndRenderPass(VkCommandBuffer cmd_buffer, const VkRenderPassBeginInfo & info)
{
	// Mark all attached images as unlocked
	auto&& views = m_StateTable->m_ResourceFramebufferSources[info.framebuffer];
	for (auto view : views)
	{
		// TODO: Is there a use for end hooking? Needs more investigation...
		(void)view;
	}
}

void ResourceInitializationPass::InitializeBuffer(VkCommandBuffer cmd_buffer, VkBuffer buffer)
{
	ResourceInitializationSRMaskWriteData data{};
	data.m_RID = GetLockUID(buffer);
	data.m_SRMask = 1;

    if ((m_GlobalStateMirror[data.m_RID] & data.m_SRMask) == data.m_SRMask)
        return;

    m_GlobalStateMirror[data.m_RID] |= data.m_SRMask;
	m_KernelSRMaskWrite.Dispatch(cmd_buffer, data);
}

void ResourceInitializationPass::InitializeImage(VkCommandBuffer cmd_buffer, VkImage image, const VkImageSubresourceRange & range)
{
	ResourceInitializationSRMaskWriteData data{};
	data.m_RID = GetLockUID(image);
	data.m_SRMask = GetImageSRMask(image, range);

    if ((m_GlobalStateMirror[data.m_RID] & data.m_SRMask) == data.m_SRMask)
        return;

    m_GlobalStateMirror[data.m_RID] |= data.m_SRMask;
	m_KernelSRMaskWrite.Dispatch(cmd_buffer, data);
}

void ResourceInitializationPass::InitializeImageView(VkCommandBuffer cmd_buffer, VkImageView view)
{
	ResourceInitializationSRMaskWriteData data{};
	data.m_RID = GetLockUID(GetImageViewKey(view));
	data.m_SRMask = GetImageViewSRMask(view);

    if ((m_GlobalStateMirror[data.m_RID] & data.m_SRMask) == data.m_SRMask)
        return;

    m_GlobalStateMirror[data.m_RID] |= data.m_SRMask;
	m_KernelSRMaskWrite.Dispatch(cmd_buffer, data);
}

uint32_t ResourceInitializationPass::GetLockUID(void * key)
{
	// Get a unique lock id
	auto&& lock_uid = m_StateOffsets[key];
	if (lock_uid == 0)
	{
		// Acquire uid
		lock_uid = static_cast<uint32_t>(m_StateOffsets.size());
	}

	return lock_uid;
}

void* ResourceInitializationPass::GetImageViewKey(VkImageView view)
{
	auto it = m_ImageViewKeys.find(view);
	if (it != m_ImageViewKeys.end())
		return it->second;

	const VkImageViewCreateInfo& info = m_StateTable->m_ResourceImageViewSources[view];

	// Note: Subresource not accounted for, part of the SRMASK instead
	return (m_ImageViewKeys[view] = reinterpret_cast<void*>(info.image));
}

uint32_t ResourceInitializationPass::GetImageViewSRMask(VkImageView view)
{
	auto it = m_ImageViewSRMasks.find(view);
	if (it != m_ImageViewSRMasks.end())
		return it->second;

	const VkImageViewCreateInfo& view_info = m_StateTable->m_ResourceImageViewSources[view];

	// Get mask
	uint32_t mask = GetImageSRMask(view_info.image, view_info.subresourceRange);

	return (m_ImageViewSRMasks[view] = mask);
}

uint32_t ResourceInitializationPass::GetImageSRMask(VkImage image, VkImageSubresourceRange range)
{
	const VkImageCreateInfo& image_info = m_StateTable->m_ResourceImageSources[image];

	// Account for remaining ranges
	if (range.layerCount == VK_REMAINING_ARRAY_LAYERS) range.layerCount = image_info.arrayLayers - range.baseArrayLayer;
	if (range.levelCount == VK_REMAINING_MIP_LEVELS) range.levelCount = image_info.mipLevels - range.baseMipLevel;

	uint32_t layer_end = range.baseArrayLayer + range.layerCount;
	uint32_t level_end = range.baseMipLevel + range.levelCount;

	// Mask of zero indicates no state validation
	uint32_t mask = 0;

	/*    LAYER
			->
		x x x x x
		x x x o x || LEVEL
		x o x x x \/
		x o x x x
	*/

	// Ensure that our tracking mask can accomodate
	if (layer_end * image_info.mipLevels + level_end > 32)
	{
		// Note: Disabled for now, this helper function gets called quite often so it results in a bit of spam
#if 0
		if (m_Table->m_CreateInfoAVA.m_LogCallback && (m_Table->m_CreateInfoAVA.m_LogSeverityMask & VK_GPU_VALIDATION_LOG_SEVERITY_WARNING))
		{
			char buffer[256];
			FormatBuffer(buffer, "Initialization instrumentation for subresourced image [%p] skipped, exceeds 32 bit tracking mask", image);
			m_Table->m_CreateInfoAVA.m_LogCallback(m_Table->m_CreateInfoAVA.m_UserData, VK_GPU_VALIDATION_LOG_SEVERITY_WARNING, __FILE__, __LINE__, buffer);
		}
#endif
	}
	else
	{
		// Mask all relevant sub-subresources (sub sub sub)
        for (uint32_t y = range.baseMipLevel; y < level_end; y++)
        {
            for (uint32_t x = range.baseArrayLayer; x < layer_end; x++)
            {
				mask |= (1u << (y * image_info.arrayLayers + x));
			}
		}
	}

	return mask;
}

void ResourceInitializationPass::InsertBatched(SCommandBufferVersion &version, SStringCache *message_cache, uint64_t key, const SDiagnosticMessageData & message, uint32_t count)
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
	msg.m_Feature = VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION;
	msg.m_Error.m_ObjectInfo.m_Name = nullptr;
	msg.m_Error.m_ObjectInfo.m_Object = nullptr;
	msg.m_Error.m_UserMarkerCount = 0;
	msg.m_Error.m_ErrorType = VK_GPU_VALIDATION_ERROR_TYPE_SUBRESOURCE_UNINITIALIZED;
	msg.m_Error.m_Message = "Reading from a potentially uninitialized subresource range";

	// Import message
	auto imported = message.GetMessage<ResourceInitializationValidationMessage>();

	// Debugging
#if RESOURCE_INITIALZIATION_PASS_SHORTSRMASK
    char buffer[512];
    FormatBuffer(buffer, "Reading from an uninitialized subresource range [ A%i : W%i ]", (uint32_t)imported.m_AccessedSRMask, (uint32_t)imported.m_WrittenSRMask);
    msg.m_Error.m_Message = message_cache->Get(buffer);
#endif

    if (imported.m_ShaderSpanGUID != UINT32_MAX && m_StateTable->m_DiagnosticRegistry->GetLocationRegistry()->GetExtractFromUID(imported.m_ShaderSpanGUID, &msg.m_Error.m_SourceExtract))
    {
        // Attempt to get associated binding
        ShaderLocationBinding binding{};
        if (m_StateTable->m_DiagnosticRegistry->GetLocationRegistry()->GetBindingMapping(imported.m_ShaderSpanGUID, VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION, &binding))
        {
            STrackedWrite descriptor = version.GetDescriptorSet(binding.m_SetIndex).GetBinding(binding.m_BindingIndex);

            // Get the object info
            msg.m_Error.m_ObjectInfo = GetDescriptorObjectInfo(m_StateTable, descriptor);
        }
	}

	m_Messages.push_back(msg);
	m_MessageLUT.insert(std::make_pair(key, m_Messages.size() - 1));
}

void ResourceInitializationPass::FreeImage(VkCommandBuffer cmd_buffer, VkImage image)
{
    ResourceInitializationSRMaskFreeData data{};
    data.m_RID = GetLockUID(image);

    m_GlobalStateMirror[data.m_RID] = 0;
    m_KernelSRMaskFree.Dispatch(cmd_buffer, data);
}

void ResourceInitializationPass::FreeBuffer(VkCommandBuffer cmd_buffer, VkBuffer buffer)
{
    ResourceInitializationSRMaskFreeData data{};
    data.m_RID = GetLockUID(buffer);

    m_GlobalStateMirror[data.m_RID] = 0;
    m_KernelSRMaskFree.Dispatch(cmd_buffer, data);
}
