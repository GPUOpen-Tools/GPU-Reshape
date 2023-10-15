// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Std
#include <initializer_list>

namespace Test {
    class IDevice : public TComponent<IDevice> {
    public:
        COMPONENT(ITestDevice);

        /// Get the name of this device
        /// \return
        virtual const char* GetName() = 0;

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
        /// \param dataSize byte size of the initial data
        /// \return resource identifier
        virtual BufferID CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, const void *data, uint64_t dataSize) = 0;

        /// Create a new texture
        /// \param type resource type
        /// \param format texel format
        /// \param width width of the texture
        /// \param height height of the texture
        /// \param depth depth of the texture
        /// \param data initial data
        /// \param dataSize byte size of the initial data
        /// \return resource identifier
        virtual TextureID CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, const void *data, uint64_t dataSize) = 0;

        /// Create a new sampler
        /// \return resource identifier
        virtual SamplerID CreateSampler() = 0;

        /// Create a new constant buffer
        /// \param byteSize size of data
        /// \param data optional, initial data
        /// \param dataSize byte size of the initial data
        /// \return resource identifier
        virtual CBufferID CreateCBuffer(uint32_t byteSize, const void* data, uint64_t dataSize) = 0;

        /// Create a resource layout
        /// \param types all types of the layout
        /// \param count number of types
        /// \return layout identifier
        virtual ResourceLayoutID CreateResourceLayout(const ResourceType *types, uint32_t count, bool isLastUnbounded = false) = 0;

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
        virtual PipelineID CreateComputePipeline(const ResourceLayoutID* layouts, uint32_t layoutCount, const void *shaderCode, uint64_t shaderSize) = 0;

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
        virtual void BindResourceSet(CommandBufferID commandBuffer, uint32_t slot, ResourceSetID resourceSet) = 0;

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

        /// Initialize all resources
        /// \param commandBuffer the command buffer to be recorded into
        virtual void InitializeResources(CommandBufferID commandBuffer) = 0;

        /// Flush and wait for all work
        virtual void Flush() = 0;

        /// Create a resource layout
        /// \param types all types of the layout
        /// \param count number of types
        /// \return layout identifier
        ResourceLayoutID CreateResourceLayout(const std::initializer_list<ResourceType>& types, bool isLastUnbounded = false) {
            return CreateResourceLayout(types.begin(), static_cast<uint32_t>(types.size()), isLastUnbounded);
        }

        /// Create a resource layout
        /// \param types all types of the layout
        /// \param count number of types
        /// \return set identifier
        ResourceSetID CreateResourceSet(ResourceLayoutID layout, const std::initializer_list<ResourceID>& resources) {
            return CreateResourceSet(layout, resources.begin(), static_cast<uint32_t>(resources.size()));
        }

        /// Create a compute pipeline
        /// \param layouts all layout definitions
        /// \param layoutCount the number of layouts
        /// \param shaderCode shader code
        /// \param shaderSize byte size of shader code
        /// \return pipeline identifier
        PipelineID CreateComputePipeline(const std::initializer_list<ResourceLayoutID>& layouts, const void *shaderCode, uint64_t shaderSize) {
            return CreateComputePipeline(layouts.begin(), static_cast<uint32_t>(layouts.size()), shaderCode, shaderSize);
        }
    };
}
