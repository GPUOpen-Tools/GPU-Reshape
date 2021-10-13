#pragma once

#include "Common.h"
#include <vector>
#include <mutex>

// Represents a push constant descriptor
struct SPushConstantDescriptor
{
	size_t m_DataOffset;
};

// Represents a wrapped top level descriptor
// Need to be able to accomodate any descriptor update mechanism
struct SDescriptor
{
	uint32_t            m_DstBinding;
	uint32_t            m_DescriptorCount;
	VkDescriptorType    m_DescriptorType;
	size_t				m_BlobOffset;
	uint32_t			m_ArrayStride;
};

// Represents a diagnostic descriptor
struct SDiagnosticDescriptor
{
	union
	{
		VkDescriptorImageInfo  m_ImageInfo;
		VkDescriptorBufferInfo m_BufferInfo;
		VkBufferView		   m_TexelBufferView;
	};
};

// Represents a wrapped descriptor pool
struct HDescriptorPool : public TDeferredOwnership<HDescriptorPool>
{
	VkDescriptorPool					m_Pool;
	std::vector<struct HDescriptorSet*> m_Sets;
	std::mutex							m_InternalLock;
	uint32_t						    m_SwapIndex;
};

// Represents a wrapped descriptor update template
struct HDescriptorUpdateTemplate : public TDeferredOwnership<HDescriptorUpdateTemplate>
{
	VkDescriptorUpdateTemplate m_Template;
	size_t					   m_TopBlobSize;
	size_t					   m_BlobSize;
	uint32_t				   m_TopCount;
	std::vector<SDescriptor>   m_Descriptors;
};

// Represents a wrapped descriptor set layout
struct HDescriptorSetLayout : public TDeferredOwnership<HDescriptorSetLayout>
{
	VkDescriptorSetLayout	   m_SetLayout;
	uint32_t				   m_TopBinding;
	uint32_t				   m_TopCount;
	std::vector<SDescriptor>   m_Descriptors;
    size_t                     m_CrossCompatibilityHash;
};

// Represents a tracked descriptor write
struct STrackedWrite
{
	uint32_t                         m_DstBinding;
	uint32_t                         m_DstArrayElement;
	uint32_t                         m_DescriptorCount;
	VkDescriptorType                 m_DescriptorType;
	union
	{
		VkDescriptorImageInfo     m_ImageInfo;
		VkDescriptorBufferInfo    m_BufferInfo;
		VkBufferView              m_TexelBufferView;
	};
};

// Represents a wrapped descriptor set
struct HDescriptorSet : public TDeferredOwnership<HDescriptorSet>
{
	VkDescriptorSet			   m_Set;
	HDescriptorSetLayout*	   m_SetLayout;
	std::vector<void*>		   m_Storage;
	std::vector<STrackedWrite> m_TrackedWrites;
	bool					   m_Valid;
    uint64_t                   m_CommitHash;
    uint64_t                   m_CommitIndex;
};