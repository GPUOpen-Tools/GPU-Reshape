#include <DiagnosticRegistry.h>

DiagnosticRegistry::DiagnosticRegistry()
{

}

void DiagnosticRegistry::Initialize(const VkGPUValidationCreateInfoAVA & create_info)
{
	m_LocationRegistry.Initialize(create_info);
}

void DiagnosticRegistry::Release()
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->Release();
	}
}

void DiagnosticRegistry::InitializePasses(VkCommandBuffer cmd_buffer)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->Initialize(cmd_buffer);
	}
}

void DiagnosticRegistry::Register(uint32_t feature_id, IDiagnosticPass * pass)
{
	PassInfo info;
	info.m_Pass = pass;
	info.m_FeatureID = feature_id;
	m_Passes.push_back(info);
}

IDiagnosticPass * DiagnosticRegistry::GetPass(uint32_t active_features, uint32_t feature_id)
{
	for (const PassInfo& pass : m_Passes)
	{
		if ((active_features & pass.m_FeatureID) != pass.m_FeatureID)
			continue;

		if (pass.m_FeatureID == feature_id)
		{
			return pass.m_Pass;
		}
	}

	return nullptr;
}

void DiagnosticRegistry::EnumerateStorage(SDiagnosticStorageInfo * storage, uint32_t * count)
{
	for (const PassInfo& pass : m_Passes)
	{
		uint32_t _count = 0;
		pass.m_Pass->EnumerateStorage(storage, &_count);

		if (storage)
			storage += _count;
		else
			*count += _count;
	}
}

void DiagnosticRegistry::EnumerateDescriptors(SDiagnosticDescriptorInfo * descriptors, uint32_t * count)
{
	for (const PassInfo& pass : m_Passes)
	{
		uint32_t _count = 0;
		pass.m_Pass->EnumerateDescriptors(descriptors, &_count);

		if (descriptors)
			descriptors += _count;
		else
			*count += _count;
	}
}

void DiagnosticRegistry::EnumeratePushConstants(SDiagnosticPushConstantInfo * constants, uint32_t * count)
{
	for (const PassInfo& pass : m_Passes)
	{
		uint32_t _count = 0;
		pass.m_Pass->EnumeratePushConstants(constants, &_count);

		if (constants)
			constants += _count;
		else
			*count += _count;
	}
}

size_t DiagnosticRegistry::UpdatePushConstants(VkCommandBuffer buffer, uint32_t feature_set, SPushConstantDescriptor * constants, uint8_t * data)
{
	size_t bytes = 0;
	for (const PassInfo& pass : m_Passes)
	{
		if ((feature_set & pass.m_FeatureID) != pass.m_FeatureID)
			continue;

		bytes += pass.m_Pass->UpdatePushConstants(buffer, constants, data);
	}
	return bytes;
}

void DiagnosticRegistry::CreateDescriptors(HDescriptorSet* set)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->CreateDescriptors(set);
	}
}

void DiagnosticRegistry::DestroyDescriptors(HDescriptorSet* set)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->DestroyDescriptors(set);
	}
}

void DiagnosticRegistry::UpdateDescriptors(HDescriptorSet* set, bool push, uint32_t feature_set, SDescriptor* top_descriptors, SDescriptor* diagnostic_descriptors, uint32_t top_count, uint8_t * blob)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->UpdateDescriptors(set, (feature_set & pass.m_FeatureID) == pass.m_FeatureID, push, top_descriptors, diagnostic_descriptors, top_count, blob);
	}
}

void DiagnosticRegistry::GenerateReport(VkGPUValidationReportAVA report)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->Report(report);
	}
}

void DiagnosticRegistry::StepReport(VkGPUValidationReportAVA report)
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->Step(report);
	}
}

void DiagnosticRegistry::Flush()
{
	for (const PassInfo& pass : m_Passes)
	{
		pass.m_Pass->Flush();
	}
}

uint32_t DiagnosticRegistry::Handle(SCommandBufferVersion& version, const SDiagnosticData * data, void * const * storage)
{
	if (data->m_MessageCount == 0)
		return 0;

	uint32_t handled = 0;

	std::lock_guard<std::mutex> guard(m_FilterLock);

	const SDiagnosticMessageData* base_message = &data->m_Messages[0];
	uint32_t base_type = base_message->m_Type;

	// Naive batching
	for (uint32_t i = 1; i < data->m_MessageCount; i++)
	{
		if (data->m_Messages[i].m_Type != base_type)
		{
			handled += m_LUT[base_type]->Handle(&m_StringCache, version, base_message, uint32_t(&data->m_Messages[i] - base_message), nullptr);

			base_message = &data->m_Messages[i];
			base_type = base_message->m_Type;;
		}
	}

	// Handle dangling messages
	const SDiagnosticMessageData* last = &data->m_Messages[data->m_MessageCount - 1];
	if (base_message != last)
	{
		handled += m_LUT[base_message->m_Type]->Handle(&m_StringCache, version, base_message, uint32_t(last - base_message) + 1, nullptr);
	}

	return handled;
}

void DiagnosticRegistry::Register(uint32_t feature_mask, SPIRV::ShaderState * state, spvtools::Optimizer * optimizer)
{
	for (const PassInfo& pass : m_Passes)
	{
		if ((feature_mask & pass.m_FeatureID) != pass.m_FeatureID)
			continue;

		pass.m_Pass->Register(state, optimizer);
	}
}

void DiagnosticRegistry::SetMessageHandler(uint16_t uid, IDiagnosticPass * handler)
{
	m_LUT[uid] = handler;
}

uint64_t DiagnosticRegistry::GetFeatureVersionUID(uint32_t feature_mask)
{
	uint64_t hash = 0;
	for (const PassInfo& pass : m_Passes)
	{
		if ((feature_mask & pass.m_FeatureID) != pass.m_FeatureID)
			continue;

        CombineHash(hash, pass.m_FeatureID);
	}
	return hash;
}

uint16_t DiagnosticRegistry::AllocateMessageUID()
{
	/*assert*/
	return m_MessageUID++;
}

uint16_t DiagnosticRegistry::AllocateStorageUID()
{
	/*assert*/
	return m_StorageUID++;
}

uint16_t DiagnosticRegistry::AllocateDescriptorUID()
{
	/*assert*/
	return m_DescriptorUID++;
}

uint16_t DiagnosticRegistry::AllocateDescriptorStorageUID()
{
	/*assert*/
	return m_DescriptorStorageUID++;
}

uint16_t DiagnosticRegistry::AllocatePushConstantUID()
{
	/*assert*/
	return m_PushConstantUID++;
}
