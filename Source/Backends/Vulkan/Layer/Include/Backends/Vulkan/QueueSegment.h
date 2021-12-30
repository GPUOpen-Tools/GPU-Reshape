#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Common
#include <Common/Assert.h>
#include <Common/ReferenceObject.h>

// Std
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct QueueSegment : public ReferenceObject {
    /// Add a referenced object to the GPU lifetime of this queue allocation
    ///   ? Note, not immediate, lifetime completion is checked when queried
    /// \param obj the referenced object, once the gpu has exhausted the allocation, the objects are released
    void AddLifetime(ReferenceObject* obj) {
        obj->AddUser();
        gpuReferences.emplace_back(obj);
    }

    /// Clear this allocation
    void Clear() {
        ASSERT(gpuReferences.empty(), "Dangling gpu references in QueueSegment");
        fence = nullptr;
        finished = false;
    }

    /// Query this allocation for completion
    /// \return true if the allocation has completed
    bool Query();

    /// Query this allocation for completion, do not invoke releasing for gpu lifetime references
    /// \return true if the allocation has completed
    bool QueryNoRelease();

    /// Parent dispatch table
    DeviceDispatchTable* table;

    /// Segment GPU -> CPU fence
    VkFence fence;

    /// GPU lifetime references
    std::vector<ReferenceObject*> gpuReferences;

private:
    /// Cached fence state
    bool finished{false};
};