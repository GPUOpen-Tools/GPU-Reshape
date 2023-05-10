#include <Backends/Vulkan/Debug.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>

VkResult Hook_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Handle type
    switch (pNameInfo->objectType) {
        default: {
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE: {
            PipelineState* state = table->states_pipeline.Get(reinterpret_cast<VkPipeline>(pNameInfo->objectHandle));

            // Get length
            size_t length = std::strlen(pNameInfo->pObjectName) + 1;

            // Copy string
            state->debugName = new (table->allocators) char[length];
            strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);
            break;
        }
        case VK_OBJECT_TYPE_IMAGE: {
            ImageState* state = table->states_image.Get(reinterpret_cast<VkImage>(pNameInfo->objectHandle));

            // Get length
            size_t length = std::strlen(pNameInfo->pObjectName) + 1;

            // Copy string
            state->debugName = new (table->allocators) char[length];
            strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

            // Inform controller of the change
            table->versioningController->CreateOrRecommitImage(state);
            break;
        }
        case VK_OBJECT_TYPE_BUFFER: {
            BufferState* state = table->states_buffer.Get(reinterpret_cast<VkBuffer>(pNameInfo->objectHandle));

            // Get length
            size_t length = std::strlen(pNameInfo->pObjectName) + 1;

            // Copy string
            state->debugName = new (table->allocators) char[length];
            strcpy_s(state->debugName, length * sizeof(char), pNameInfo->pObjectName);

            // Inform controller of the change
            table->versioningController->CreateOrRecommitBuffer(state);
            break;
        }
    }

    // Pass down callchain
    return table->next_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}
