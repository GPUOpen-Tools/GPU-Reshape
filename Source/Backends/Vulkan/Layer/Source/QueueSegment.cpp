#include <Backends/Vulkan/QueueSegment.h>
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

bool QueueSegment::Query() {
    bool state = QueryNoRelease();
    if (state) {
        // Release all references
        for (ReferenceObject* obj : gpuReferences) {
            destroyRef(obj, table->allocators);
        }

        gpuReferences.clear();
    }

    return state;
}

bool QueueSegment::QueryNoRelease() {
    if (finished) {
        return true;
    }

    // Query latest
    return finished = (table->next_vkGetFenceStatus(table->object, fence) == VK_SUCCESS);
}
