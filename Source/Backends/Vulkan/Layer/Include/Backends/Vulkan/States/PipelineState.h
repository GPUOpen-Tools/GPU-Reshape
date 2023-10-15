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

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/States/PipelineType.h>

// Common
#include <Common/Containers/ReferenceObject.h>

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <atomic>
#include <map>
#include <mutex>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderModuleState;
struct PipelineLayoutState;
struct RenderPassState;

struct PipelineState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineState();

    /// Add an instrument to this module
    /// \param featureBitSet the enabled feature set
    /// \param pipeline the pipeline in question
    void AddInstrument(uint64_t featureBitSet, VkPipeline pipeline) {
        std::lock_guard lock(mutex);
        instrumentObjects[featureBitSet] = pipeline;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    VkPipeline GetInstrument(uint64_t featureBitSet) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(featureBitSet);
        if (it == instrumentObjects.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Check if there's an instrumentation request
    /// \return true if there's a request
    bool HasInstrumentationRequest() const {
        return instrumentationInfo.featureBitSet != 0;
    }

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    ///  ! May be nullptr if the top pipeline has been destroyed
    VkPipeline object{VK_NULL_HANDLE};

    /// Type of the pipeline
    PipelineType type;

    /// Replaced pipeline object, fx. instrumented version
    std::atomic<VkPipeline> hotSwapObject{VK_NULL_HANDLE};

    /// Layout for this pipeline
    PipelineLayoutState* layout{nullptr};

    ///Referenced shader modules
    std::vector<ShaderModuleState*> shaderModules;

    /// Optional debug name
    char* debugName{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Shader dependent instrumentation info
    DependentInstrumentationInfo dependentInstrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<uint64_t, VkPipeline> instrumentObjects;

    /// Module specific lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};

struct GraphicsPipelineState : public PipelineState {
    /// Recreation info
    VkGraphicsPipelineCreateInfoDeepCopy createInfoDeepCopy;

    /// Render pass for this pipeline
    RenderPassState* renderPass{nullptr};
};

struct ComputePipelineState : public PipelineState {
    /// Recreation info
    VkComputePipelineCreateInfoDeepCopy createInfoDeepCopy;
};
