// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/ImageState.h>

// Backend
#include <Backend/Resource/ResourceInfo.h>

// Common
#include <Common/Assert.h>

/// Get the resource info from a mapping
/// \param mapping given mapping
/// \param isVolumetric is the resource using volumetric mapping?
/// \return resource info
static ResourceInfo GetResourceInfoFor(const VirtualResourceMapping& mapping, bool isVolumetric) {
    // Construct without descriptor
    switch (static_cast<Backend::IL::ResourceTokenType>(mapping.token.type)) {
        default:
            ASSERT(false, "Unexpected type");
        return {};
        case Backend::IL::ResourceTokenType::Texture:
            return ResourceInfo::Texture(mapping.token, isVolumetric);
        case Backend::IL::ResourceTokenType::Buffer:
            return ResourceInfo::Buffer(mapping.token);
    }
}

/// Get the resource info from a mapping
/// \param state resource state
/// \return resource info
static ResourceInfo GetResourceInfoFor(const ImageState *state) {
    return GetResourceInfoFor(state->virtualMappingTemplate, state->createInfo.extent.depth > 1u);
}

/// Get the resource info from a mapping
/// \param state resource state
/// \return resource info
static ResourceInfo GetResourceInfoFor(const ImageViewState *state) {
    return GetResourceInfoFor(state->virtualMapping, state->parent->createInfo.extent.depth > 1u);
}

/// Get a buffer placement descriptor
/// \param state the target state
/// \param bufferRowLength byte length of each row
/// \param bufferImageHeight the height of the image or region
/// \return descriptor
static BufferPlacedDescriptor GetBufferPlacedDescriptor(ImageState* state, uint32_t bufferRowLength, uint32_t bufferImageHeight) {
    // Zero is allowed, assume tightly packed coordinates
    if (bufferRowLength == 0 || bufferImageHeight == 0) {
        ASSERT(state->virtualMappingTemplate.token.formatSize != 0, "Unexpected width");
        bufferRowLength = state->createInfo.extent.width * state->virtualMappingTemplate.token.formatSize;
        bufferImageHeight = state->createInfo.extent.height;
    }

    // Create descriptor
    BufferPlacedDescriptor out;
    out.rowLength = bufferRowLength;
    out.imageHeight = bufferImageHeight;
    return out;
}

/// Expand a full subresource range
/// \param state the source state
/// \param range the range to expand
/// \return expanded range
static VkImageSubresourceRange ExpandImageSubresourceRange(const ImageState* state, const VkImageSubresourceRange& range) {
    VkImageSubresourceRange expanded = range;

    // Expand levels
    if (expanded.levelCount == VK_REMAINING_MIP_LEVELS) {
        expanded.levelCount = state->createInfo.mipLevels - expanded.baseMipLevel;
    }

    // Expand layers
    if (expanded.layerCount == VK_REMAINING_ARRAY_LAYERS) {
        expanded.layerCount = state->createInfo.arrayLayers - expanded.baseArrayLayer;
    }

    return expanded;
}

/// Expand a subresource layer range
/// \param state the source state
/// \param range the range to expand
/// \return expanded range
static VkImageSubresourceLayers ExpandImageSubresourceLayers(const ImageState* state, const VkImageSubresourceLayers& range) {
    VkImageSubresourceLayers expanded = range;

    // Expand layers
    if (expanded.layerCount == VK_REMAINING_ARRAY_LAYERS) {
        expanded.layerCount = state->createInfo.arrayLayers - expanded.baseArrayLayer;
    }

    return expanded;
}