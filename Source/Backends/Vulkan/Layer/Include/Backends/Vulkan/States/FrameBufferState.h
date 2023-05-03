#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;
struct ImageViewState;

struct FrameBufferState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~FrameBufferState();

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User frame buffer
    VkFramebuffer object{VK_NULL_HANDLE};

    /// All image views
    std::vector<ImageViewState*> imageViews;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
