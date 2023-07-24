#pragma once

// Layer
#include <Backends/Vulkan/States/SamplerState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Resource/DescriptorResourceMapping.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

/// Get the virtual resource mapping
/// \param table parent table
/// \param type descriptor type
/// \param info descriptor type
static VirtualResourceMapping GetVirtualResourceMapping(DeviceDispatchTable* table, VkDescriptorType type, const VkDescriptorImageInfo& info) {
    VirtualResourceMapping mapping{};

    // Handle type
    switch (type) {
        default: {
            ASSERT(false, "Invalid type");
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER: {
            if (info.sampler) {
                mapping = table->states_sampler.Get(info.sampler)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullSampler;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler);
                mapping.srb  = 0x1;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
            if (info.imageView) {
                mapping = table->states_imageView.Get(info.imageView)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullTexture;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
                mapping.srb  = 0x1;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            if (info.imageView) {
                mapping = table->states_imageView.Get(info.imageView)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullTexture;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
                mapping.srb  = 0x1;
            }
            break;
        }
    }

    // OK
    return mapping;
}

/// Get the virtual resource mapping
/// \param table parent table
/// \param type descriptor type
/// \param info descriptor type
static VirtualResourceMapping GetVirtualResourceMapping(DeviceDispatchTable* table, VkDescriptorType type, VkBufferView info) {
    VirtualResourceMapping mapping{};
    
    // Handle type
    switch (type) {
        default: {
            ASSERT(false, "Invalid type");
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
            if (info) {
                mapping = table->states_bufferView.Get(info)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullBuffer;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
                mapping.srb  = 0x1;
            }
            break;
        }
    }

    // OK
    return mapping;
}

/// Get the virtual resource mapping
/// \param table parent table
/// \param type descriptor type
/// \param info descriptor type
static VirtualResourceMapping GetVirtualResourceMapping(DeviceDispatchTable* table, VkDescriptorType type, VkDescriptorBufferInfo info) {
    VirtualResourceMapping mapping{};
    
    // Handle type
    switch (type) {
        default: {
            ASSERT(false, "Invalid type");
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
            if (info.buffer) {
                mapping = table->states_buffer.Get(info.buffer)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullCBuffer;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::CBuffer);
                mapping.srb  = 0x1;
            }
            break;
        }
    }

    // OK
    return mapping;
}

/// Get the virtual resource mapping
/// \param table parent table
/// \param write the write information
/// \param descriptorIndex the current index to fill
static VirtualResourceMapping GetVirtualResourceMapping(DeviceDispatchTable* table, const VkWriteDescriptorSet& write, uint32_t descriptorIndex) {
    // Default invalid mapping
    VirtualResourceMapping mapping{};
    mapping.puid = IL::kResourceTokenPUIDMask;

    // Handle type
    switch (write.descriptorType) {
        default: {
            // Perhaps handled in the future
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:  {
            mapping = GetVirtualResourceMapping(table, write.descriptorType, write.pImageInfo[descriptorIndex]);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
            if (write.pTexelBufferView[descriptorIndex]) {
                mapping = table->states_bufferView.Get(write.pTexelBufferView[descriptorIndex])->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullBuffer;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
                mapping.srb  = 0x1;
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
            if (write.pBufferInfo[descriptorIndex].buffer) {
                mapping = table->states_buffer.Get(write.pBufferInfo[descriptorIndex].buffer)->virtualMapping;
            } else if (table->physicalDeviceRobustness2Features.nullDescriptor) {
                mapping.puid = IL::kResourceTokenPUIDReservedNullCBuffer;
                mapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::CBuffer);
                mapping.srb  = 0x1;
            }
            break;
        }
    }

    // OK
    return mapping;
}

/// Get the virtual resource mapping
/// \param table parent table
/// \param descriptorType the descriptor type
/// \param descriptorData opaque data, must be for the type
static VirtualResourceMapping GetVirtualResourceMapping(DeviceDispatchTable* table, VkDescriptorType descriptorType, const void* descriptorData) {
    // Default invalid mapping
    VirtualResourceMapping mapping{};
    mapping.puid = IL::kResourceTokenPUIDMask;

    // Handle type
    switch (descriptorType) {
        default: {
            // Perhaps handled in the future
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
            auto&& typeData = *static_cast<const VkDescriptorImageInfo*>(descriptorData);
            mapping = GetVirtualResourceMapping(table, descriptorType, typeData);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
            auto&& typeData = *static_cast<const VkBufferView*>(descriptorData);
            mapping = GetVirtualResourceMapping(table, descriptorType, typeData);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
            auto&& typeData = *static_cast<const VkDescriptorBufferInfo*>(descriptorData);
            mapping = GetVirtualResourceMapping(table, descriptorType, typeData);
            break;
        }
    }

    // OK
    return mapping;
}

