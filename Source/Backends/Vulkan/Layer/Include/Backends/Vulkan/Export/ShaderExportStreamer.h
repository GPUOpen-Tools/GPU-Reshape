#pragma once

// Common
#include <Common/IComponent.h>

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Forward declarations
struct DeviceDispatchTable;

class ShaderExportStreamer : public IComponent {
public:
    COMPONENT(ShaderExportStreamer);

    ShaderExportStreamer(DeviceDispatchTable* table);

    void Install();

    void BeginCommandBuffer(VkCommandBuffer commandBuffer);

    void SyncPoint();

private:
    DeviceDispatchTable* table;
};
