#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/CommandBufferObject.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(queue));

    // Unwrapped submits
    auto* vkSubmits = ALLOCA_ARRAY(VkSubmitInfo, submitCount);

    // Number of command buffers
    uint32_t commandBufferCount = 0;
    for (uint32_t i = 0; i < submitCount; i++) {
        commandBufferCount += pSubmits[i].commandBufferCount;
    }

    // Unwrapped command buffers
    auto* vkCommandBuffers = ALLOCA_ARRAY(VkCommandBuffer, commandBufferCount);

    // Unwrap all internal states
    for (uint32_t i = 0; i < submitCount; i++) {
        // Unwrap the command buffers
        for (uint32_t bufferIndex = 0; bufferIndex < pSubmits[i].commandBufferCount; bufferIndex++) {
            auto* unwrapped = reinterpret_cast<CommandBufferObject*>(pSubmits[i].pCommandBuffers[bufferIndex]);
            vkCommandBuffers[bufferIndex] = unwrapped->object;
        }

        // Copy non wrapped info
        vkSubmits[i].pNext                = pSubmits[i].pNext;
        vkSubmits[i].sType                = pSubmits[i].sType;
        vkSubmits[i].pSignalSemaphores    = pSubmits[i].pSignalSemaphores;
        vkSubmits[i].signalSemaphoreCount = pSubmits[i].signalSemaphoreCount;
        vkSubmits[i].pWaitSemaphores      = pSubmits[i].pWaitSemaphores;
        vkSubmits[i].waitSemaphoreCount   = pSubmits[i].waitSemaphoreCount;
        vkSubmits[i].pWaitDstStageMask    = pSubmits[i].pWaitDstStageMask;

        // Assign unwrapped states
        vkSubmits[i].commandBufferCount   = pSubmits[i].commandBufferCount;
        vkSubmits[i].pCommandBuffers      = vkCommandBuffers;

        // Advance buffers
        vkCommandBuffers += vkSubmits[i].commandBufferCount;
    }

    // Pass down callchain
    VkResult result = table->next_vkQueueSubmit(queue, submitCount, vkSubmits, fence);
    if (result != VK_SUCCESS) {
        return result;
    }

    // OK
    return VK_SUCCESS;
}
