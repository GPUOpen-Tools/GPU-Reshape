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

#include <Backends/Vulkan/Debug.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>

VkResult Hook_vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Name copy
    VkDebugMarkerObjectNameInfoEXT nameInfo = *pNameInfo;

    // Handle type
    switch (pNameInfo->objectType) {
        default: {
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE: {
            if (PipelineState* state = table->states_pipeline.TryGet(reinterpret_cast<VkPipeline>(pNameInfo->object))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);
            }
            break;
        }
        case VK_OBJECT_TYPE_SHADER_MODULE: {
            if (ShaderModuleState* state = table->states_shaderModule.TryGet(reinterpret_cast<VkShaderModule>(pNameInfo->object))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);
            }
            break;
        }
        case VK_OBJECT_TYPE_IMAGE: {
            if (ImageState* state = table->states_image.TryGet(reinterpret_cast<VkImage>(pNameInfo->object))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

                // Inform controller of the change
                table->versioningController->CreateOrRecommitImage(state);
            }
            break;
        }
        case VK_OBJECT_TYPE_BUFFER: {
            if (BufferState* state = table->states_buffer.TryGet(reinterpret_cast<VkBuffer>(pNameInfo->object))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

                // Inform controller of the change
                table->versioningController->CreateOrRecommitBuffer(state);
            }
            break;
        }
        case VK_OBJECT_TYPE_COMMAND_BUFFER: {
            if (const auto* commandBuffer = reinterpret_cast<const CommandBufferObject*>(pNameInfo->object)) {
                // Unwrap object
                nameInfo.object = reinterpret_cast<uint64_t>(commandBuffer->object);
            }
            break;
        }
    }

    // Pass down callchain
    return table->next_vkDebugMarkerSetObjectNameEXT(device, &nameInfo);
}

VkResult Hook_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Name copy
    VkDebugUtilsObjectNameInfoEXT nameInfo = *pNameInfo;

    // Handle type
    switch (pNameInfo->objectType) {
        default: {
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE: {
            if (PipelineState* state = table->states_pipeline.TryGet(reinterpret_cast<VkPipeline>(pNameInfo->objectHandle))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);
            }
            break;
        }
        case VK_OBJECT_TYPE_SHADER_MODULE: {
            if (ShaderModuleState* state = table->states_shaderModule.TryGet(reinterpret_cast<VkShaderModule>(pNameInfo->objectHandle))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);
            }
            break;
        }
        case VK_OBJECT_TYPE_IMAGE: {
            if (ImageState* state = table->states_image.TryGet(reinterpret_cast<VkImage>(pNameInfo->objectHandle))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

                // Inform controller of the change
                table->versioningController->CreateOrRecommitImage(state);
            }
            break;
        }
        case VK_OBJECT_TYPE_BUFFER: {
            if (BufferState* state = table->states_buffer.TryGet(reinterpret_cast<VkBuffer>(pNameInfo->objectHandle))) {
                // Get length
                size_t length = std::strlen(pNameInfo->pObjectName) + 1;

                // Copy string
                state->debugName = new (table->allocators) char[length];
                strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

                // Inform controller of the change
                table->versioningController->CreateOrRecommitBuffer(state);
            }
            break;
        }
        case VK_OBJECT_TYPE_COMMAND_BUFFER: {
            if (const auto* commandBuffer = reinterpret_cast<const CommandBufferObject*>(pNameInfo->objectHandle)) {
                // Unwrap object
                nameInfo.objectHandle = reinterpret_cast<uint64_t>(commandBuffer->object);
            }
            break;
        }
    }

    // Pass down callchain
    return table->next_vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}
