#pragma once

// Common
#include <Common/IComponent.h>

// Backend
#include <Backend/IL/Format.h>
#include <Backend/IL/TextureDimension.h>

// Device
#include "ID.h"
#include "QueueType.h"
#include "DeviceInfo.h"
#include "ResourceType.h"

namespace Test {
    class IDevice : public IComponent {
    public:
        COMPONENT(ITestDevice);

        /// Install this device
        /// \param info device information
        virtual void Install(const DeviceInfo& info) = 0;

        /// Get a queue
        /// \param type type of the queue
        /// \return invalid if not available
        virtual QueueID GetQueue(QueueType type) = 0;

        /// Create a new texel buffer
        /// \param type resource type
        /// \param format texel format
        /// \param size resource byte size
        /// \param data initial data
        /// \return resource identifier
        virtual BufferID CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, void *data) = 0;

        /// Create a new texture
        /// \param type resource type
        /// \param format texel format
        /// \param width width of the texture
        /// \param height height of the texture
        /// \param depth depth of the texture
        /// \param data initial data
        /// \return resource identifier
        virtual TextureID CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, void *data) = 0;

        /// Create a resource layout
        /// \param types all types of the layout
        /// \param count number of types
        /// \return layout identifier
        virtual ResourceLayoutID CreateResourceLayout(const ResourceType *types, uint32_t count) = 0;

        /// Create a resource layout
        /// \param types all types of the layout
        /// \param count number of types
        /// \return set identifier
        virtual ResourceSetID CreateResourceSet(ResourceLayoutID layout, const ResourceID *resources, uint32_t count) = 0;

        /// Create a compute pipeline
        /// \param layouts all layout definitions
        /// \param layoutCount the number of layouts
        /// \param shaderCode shader code
        /// \param shaderSize byte size of shader code
        /// \return pipeline identifier
        virtual PipelineID CreateComputePipeline(const ResourceLayoutID* layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) = 0;

        /// Createa  new command buffer
        /// \param type submission queue type
        /// \return command buffer identifier
        virtual CommandBufferID CreateCommandBuffer(QueueType type) = 0;

        /// Begin a command buffer
        /// \param commandBuffer the command buffer
        virtual void BeginCommandBuffer(CommandBufferID commandBuffer) = 0;

        /// End a command buffer
        /// \param commandBuffer the command buffer
        virtual void EndCommandBuffer(CommandBufferID commandBuffer) = 0;

        /// Bind a pipeline
        /// \param commandBuffer command buffer to be bound to
        /// \param pipeline pipeline to bind
        virtual void BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) = 0;

        /// Bind a resource set
        /// \param commandBuffer the command buffer to be bound to
        /// \param resourceSet the resource set to bind
        virtual void BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) = 0;

        /// Dispatch a compute pipeline
        /// \param commandBuffer command buffer
        /// \param x group count x
        /// \param y group count y
        /// \param z group count z
        virtual void Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) = 0;

        /// Submit a command buffer onto a given queue
        /// \param queue the queue to submit to
        /// \param commandBuffer the command buffer to be submitted
        virtual void Submit(QueueID queue, CommandBufferID commandBuffer) = 0;

        /// Flush and wait for all work
        virtual void Flush() = 0;
    };
}