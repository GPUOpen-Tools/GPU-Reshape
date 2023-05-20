#pragma once

// Layer
#include "Objects/CommandBufferObject.h"
#include "Export/StreamState.h"

class CommandBufferRenderPassScope {
public:
    CommandBufferRenderPassScope(DeviceDispatchTable* table, VkCommandBuffer commandBuffer, ShaderExportRenderPassState* renderPassState) : table(table), renderPassState(renderPassState), commandBuffer(commandBuffer) {
        // Temporarily end render pass
        if (renderPassState->insideRenderPass) {
            table->commandBufferDispatchTable.next_vkCmdEndRenderPass(commandBuffer);
        }
    }

    ~CommandBufferRenderPassScope() {
        // Reconstruct render pass if needed
        if (renderPassState->insideRenderPass) {
            table->commandBufferDispatchTable.next_vkCmdBeginRenderPass(
                commandBuffer,
                &renderPassState->deepCopy.createInfo,
                renderPassState->subpassContents
            );
        }
    }

private:
    DeviceDispatchTable* table;
    ShaderExportRenderPassState* renderPassState;
    VkCommandBuffer commandBuffer;
};
