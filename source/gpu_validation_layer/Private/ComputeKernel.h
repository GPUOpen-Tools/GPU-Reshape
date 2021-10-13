#pragma once

#include "Common.h"
#include "DiagnosticAllocator.h"
#include <vector>

// Kernel creation info
struct ComputeKernelInfo
{
    ComputeKernelInfo() : m_ShaderBlobSize(0), m_DescriptorTypeCount(0), m_PCByteSpan(0)
    {
        /* poof */
    }

    /**
     * Set the kernel binary
     * @param[in] blob the compile time length SPIRV blob
     */
	template<size_t LENGTH>
	void Kernel(uint8_t const (&blob)[LENGTH])
	{
		m_ShaderBlob = blob;
		m_ShaderBlobSize = LENGTH;
	}

    /**
     * Set the input descriptor types
     * @param[in] types the compile time length descriptor types
     */
	template<size_t LENGTH>
	void DescriptorTypes(VkDescriptorType const (&types)[LENGTH])
	{
		m_DescriptorTypes = types;
		m_DescriptorTypeCount = LENGTH;
	}

	const uint8_t*			m_ShaderBlob;           ///< The SPIRV binary blob
	size_t					m_ShaderBlobSize;       ///< The byte size of the binary blob
	const VkDescriptorType* m_DescriptorTypes;      ///< The descriptor types
	uint32_t				m_DescriptorTypeCount;  ///< The number of descriptor types
	uint32_t				m_PCByteSpan;           ///< The immediate push constant byte span
};

// Kernel descriptor update info
union ComputeKernelDescriptor
{
	VkDescriptorImageInfo	m_ImageInfo;        ///< Upon an image view descriptor
	VkDescriptorBufferInfo	m_BufferInfo;       ///< Upon a buffer descriptor
	VkBufferView			m_TexelBufferInfo;  ///< Upon a (texel) buffer view descriptor
};

class ComputeKernel
{
public:
    /**
     * Initialize this kernel
     * @param[in] device the vulkan device
     * @param[in] info the kernel creation info
     */
	VkResult Initialize(VkDevice device, const ComputeKernelInfo& info);

    /**
     * Destroy this kernel
     */
	void Destroy();

    /**
     * Update kernel descriptors
     *  ! Must not be in flight
     * @param[in] descriptors the filled descriptors, must match the descriptor type count during creation
     */
	void UpdateDescriptors(ComputeKernelDescriptor* descriptors);

    /**
     * Dispatch this kernel
     * @param[in] cmd_buffer the vulkan command buffer
     * @param[in] data the push constant data
     * @param[in] size the length of the push constant data
     * @param[in] x dispatch thread group <x>
     * @param[in] y dispatch thread group <y>
     * @param[in] z dispatch thread group <z>
     */
    void Dispatch(VkCommandBuffer cmd_buffer, const void* data, uint32_t size, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);

    /**
     * Dispatch this kernel
     * @param[in] cmd_buffer the vulkan command buffer
     * @param[in] set the descriptor set to be bound
     * @param[in] data the push constant data
     * @param[in] size the length of the push constant data
     * @param[in] x dispatch thread group <x>
     * @param[in] y dispatch thread group <y>
     * @param[in] z dispatch thread group <z>
     */
    void Dispatch(VkCommandBuffer cmd_buffer, VkDescriptorSet set, const void* data, uint32_t size, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);

    /**
     * Get the descriptor set layout
     * @return a cross-pipeline compatible layout
     */
	VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
	    return m_SetLayout;
    }

public:
    /**
     * Dispatch this kernel
     * @param[in] cmd_buffer the vulkan command buffer
     * @param[in] data the push constant data
     * @param[in] x dispatch thread group <x>
     * @param[in] y dispatch thread group <y>
     * @param[in] z dispatch thread group <z>
     */
	template<typename T>
	void Dispatch(VkCommandBuffer cmd_buffer, const T& data, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1)
	{
		Dispatch(cmd_buffer, &data, sizeof(T), x, y, z);
	}

    /**
     * Dispatch this kernel
     * @param[in] cmd_buffer the vulkan command buffer
     * @param[in] set the descriptor set to be bound
     * @param[in] data the push constant data
     * @param[in] size the length of the push constant data
     * @param[in] x dispatch thread group <x>
     * @param[in] y dispatch thread group <y>
     * @param[in] z dispatch thread group <z>
     */
    template<typename T>
    void Dispatch(VkCommandBuffer cmd_buffer, VkDescriptorSet set,  const T& data, uint32_t x = 1, uint32_t y = 1, uint32_t z = 1)
    {
        Dispatch(cmd_buffer, set, &data, sizeof(T), x, y, z);
    }

private:
	VkDevice									m_Device;           ///< The vulkan device
	VkPipelineLayout							m_PipelineLayout;   ///< The singular pipeline layout
	VkPipeline									m_Pipeline;         ///< The kernel pipeline
	VkDescriptorSetLayout						m_SetLayout;        ///< The singular descriptor set layout
	SDiagnosticDescriptorBinding				m_SetBinding{};     ///< Allocation binding of respective set
	std::vector<VkDescriptorSetLayoutBinding>	m_Bindings;         ///< Creation bindings
};