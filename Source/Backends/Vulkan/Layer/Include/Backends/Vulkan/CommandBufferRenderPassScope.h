#pragma once

// Layer
#include "Objects/CommandBufferObject.h"
#include "Export/StreamState.h"

class CommandBufferRenderPassScope {
public:
    CommandBufferRenderPassScope(CommandBufferObject* object) : object(object) {
        // Temporarily end render pass
        if (object->streamState->renderPass.insideRenderPass) {
            object->dispatchTable.next_vkCmdEndRenderPass(object->object);
        }
    }

    ~CommandBufferRenderPassScope() {
        // Reconstruct render pass if needed
        if (object->streamState->renderPass.insideRenderPass) {
            object->dispatchTable.next_vkCmdBeginRenderPass(
                object->object,
                &object->streamState->renderPass.deepCopy.createInfo,
                object->streamState->renderPass.subpassContents
            );
        }
    }

private:
    CommandBufferObject* object;
};
