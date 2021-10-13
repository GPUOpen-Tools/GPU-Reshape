#include <ComputeKernel.h>
#include <DispatchTables.h>
#include <StateTables.h>

VkResult ComputeKernel::Initialize(VkDevice device, const ComputeKernelInfo & info)
{
	m_Device = device;

	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(device));

	VkResult result;

	// Prepare bindings
	m_Bindings.resize(info.m_DescriptorTypeCount);
	for (uint32_t i = 0; i < info.m_DescriptorTypeCount; i++)
	{
		VkDescriptorSetLayoutBinding& binding = m_Bindings[i] = {};
		binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		binding.descriptorType = info.m_DescriptorTypes[i];
		binding.binding = i;
		binding.descriptorCount = 1;
	}

	// Attempt to create set layout
	VkDescriptorSetLayoutCreateInfo set_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	set_layout_info.pBindings = m_Bindings.data();
	set_layout_info.bindingCount = (uint32_t)m_Bindings.size();
	if ((result = table->m_CreateDescriptorSetLayout(m_Device, &set_layout_info, nullptr, &m_SetLayout)) != VK_SUCCESS)
	{
		return result;
	}

	// Push constant range
	VkPushConstantRange pc_range;
	pc_range.offset = 0;
	pc_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pc_range.size = info.m_PCByteSpan;

	// Attempt to create pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &m_SetLayout;
	pipeline_layout_info.pPushConstantRanges = &pc_range;
	pipeline_layout_info.pushConstantRangeCount = (info.m_PCByteSpan > 0);
	if ((result = table->m_CreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_PipelineLayout)) != VK_SUCCESS)
	{
		return result;
	}

	// Attempt to allocate set
	VkDescriptorSetAllocateInfo set_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	set_info.descriptorSetCount = 1;
	set_info.pSetLayouts = &m_SetLayout;
	if ((result = state->m_DiagnosticAllocator->AllocateDescriptorSet(set_info, &m_SetBinding)) != VK_SUCCESS)
	{
		return result;
	}

	// Not tied to the pipeline
	VkShaderModule shader_module;

	// Create temporary shader module
	VkShaderModuleCreateInfo sm_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	sm_info.codeSize = info.m_ShaderBlobSize;
	sm_info.pCode = reinterpret_cast<const uint32_t*>(info.m_ShaderBlob);
	if ((result = table->m_CreateShaderModule(device, &sm_info, nullptr, &shader_module)) != VK_SUCCESS)
	{
		return result;
	}

	// Attempt to create pipeline layout
	VkComputePipelineCreateInfo pipe_info{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipe_info.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	pipe_info.stage.module = shader_module;
	pipe_info.stage.pName = "main";
	pipe_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipe_info.layout = m_PipelineLayout;
	if ((result = table->m_CreateComputePipelines(m_Device, nullptr, 1, &pipe_info, nullptr, &m_Pipeline)) != VK_SUCCESS)
	{
		return result;
	}

	// Cleanup
	table->m_DestroyShaderModule(device, shader_module, nullptr);

	// OK
	return VK_SUCCESS;
}

void ComputeKernel::Destroy()
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
	DeviceStateTable* state = DeviceStateTable::Get(GetKey(m_Device));

	// Destroy set
	VkResult result;
	if (m_SetBinding.m_Set)
	{
		if ((result = state->m_DiagnosticAllocator->FreeDescriptorSet(m_SetBinding)) != VK_SUCCESS)
		{
			return;
		}
	}

	// Destroy states
	table->m_DestroyPipeline(m_Device, m_Pipeline, nullptr);
	table->m_DestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	table->m_DestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);
}

void ComputeKernel::UpdateDescriptors(ComputeKernelDescriptor * descriptors)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));

	// Translate writes
	auto writes = ALLOCA_ARRAY(VkWriteDescriptorSet, m_Bindings.size());
	for (size_t i = 0; i < m_Bindings.size(); i++)
	{
		auto&& write = writes[i] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET  };
		write.dstSet = m_SetBinding.m_Set;
		write.descriptorCount = 1;
		write.descriptorType = m_Bindings[i].descriptorType;
		write.pBufferInfo = &descriptors[i].m_BufferInfo;
		write.pImageInfo = &descriptors[i].m_ImageInfo;
		write.pTexelBufferView = &descriptors[i].m_TexelBufferInfo;
	}

	// Update the set
	table->m_UpdateDescriptorSets(m_Device, static_cast<uint32_t>(m_Bindings.size()), writes, 0, nullptr);
}

void ComputeKernel::Dispatch(VkCommandBuffer cmd_buffer, const void* data, uint32_t size, uint32_t x, uint32_t y, uint32_t z)
{
	DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
    CommandStateTable* cmd_state = CommandStateTable::Get(cmd_buffer);

    // Bind the states if the internal pipeline is mismatched
    // MIPE: Disabled state filtering, a mismatched set is bound somewhere...
    // if (!cmd_state || cmd_state->m_ActiveInternalPipelines[VK_PIPELINE_BIND_POINT_COMPUTE] != m_Pipeline)
    {
	    table->m_CmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	    table->m_BindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_SetBinding.m_Set, 0, nullptr);

        // Set the internal pipeline
		if (cmd_state)
		{
			cmd_state->m_ActiveInternalPipelines[VK_PIPELINE_BIND_POINT_COMPUTE] = m_Pipeline;
		}
    }

	// Push data
	table->m_CmdPushConstants(cmd_buffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);

	// Execute the kernel
	table->m_CmdDispatch(cmd_buffer, x, y, z);
}

void ComputeKernel::Dispatch(VkCommandBuffer cmd_buffer, VkDescriptorSet set, const void *data, uint32_t size, uint32_t x, uint32_t y, uint32_t z)
{
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetKey(m_Device));
    CommandStateTable* cmd_state = CommandStateTable::Get(cmd_buffer);

    // Bind the states if the internal pipeline is mismatched
    // MIPE: Disabled state filtering, a mismatched set is bound somewhere...
    // if (!cmd_state || cmd_state->m_ActiveInternalPipelines[VK_PIPELINE_BIND_POINT_COMPUTE] != m_Pipeline)
    {
        table->m_CmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
        table->m_BindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set, 0, nullptr);

        // Set the internal pipeline
		if (cmd_state)
		{
			cmd_state->m_ActiveInternalPipelines[VK_PIPELINE_BIND_POINT_COMPUTE] = m_Pipeline;
		}
    }

    // Push data
    table->m_CmdPushConstants(cmd_buffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);

    // Execute the kernel
    table->m_CmdDispatch(cmd_buffer, x, y, z);
}
