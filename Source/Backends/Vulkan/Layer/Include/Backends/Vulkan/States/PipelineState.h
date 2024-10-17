// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>

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

/// Instrumentation state for the default pipeline object
static constexpr uint64_t kDefaultPipelineStateHash = UINT64_MAX;

struct PipelineState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineState();

    /// Release all host resources
    void ReleaseHost() override;

    /// Add an instrument to this module
    /// \param hash the pipeline hash
    /// \param pipeline the pipeline in question
    void AddInstrument(uint64_t hash, VkPipeline pipeline) {
        std::lock_guard lock(mutex);
        instrumentObjects[hash] = pipeline;
    }

    /// Reserve an instrument to be added later, defaults to nullptr
    /// \param hash the pipeline hash
    /// \return true if added, false if already present
    bool Reserve(uint64_t hash) {
        ASSERT(hash, "Invalid instrument reservation");
    
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(hash);
        if (it == instrumentObjects.end()) {
            instrumentObjects[hash] = nullptr;
            return true;
        }

        return false;
    }

    /// Check if instrument is present
    /// \param hash the pipeline hash
    /// \return false if not found
    bool HasInstrument(uint64_t hash) {
        if (hash == kDefaultPipelineStateHash) {
            return true; 
        }
        
        std::lock_guard lock(mutex);
        return instrumentObjects.count(hash) > 0;
    }

    /// Get an instrument
    /// \param hash the pipeline hash
    /// \return nullptr if not found
    VkPipeline GetInstrument(uint64_t hash) {
        // Default object?
        if (hash == kDefaultPipelineStateHash) {
            return object;
        }
        
        std::lock_guard lock(mutex);
        
        auto&& it = instrumentObjects.find(hash);
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

    /// Get the dependent instrumentation key index of a shader module
    /// \param state state to query
    /// \return always valid
    uint64_t GetDependentIndex(const ShaderModuleState* state) const {
        uint64_t dependentIndex = std::ranges::find(referencedShaderModules, state) - referencedShaderModules.begin();
        ASSERT(dependentIndex < referencedShaderModules.size(), "Dependent object not found");
        return dependentIndex;
    }

    /// Get the dependent instrumentation key index of a pipeline library state
    /// \param state state to query
    /// \return always valid
    uint64_t GetDependentIndex(const PipelineState* state) const {
        uint64_t dependentIndex = std::ranges::find(pipelineLibraries, state) - pipelineLibraries.begin();
        ASSERT(dependentIndex < pipelineLibraries.size(), "Dependent object not found");
        return dependentIndex;
    }

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    ///  ! May be nullptr if the top pipeline has been destroyed
    VkPipeline object{VK_NULL_HANDLE};

    /// Type of the pipeline
    PipelineType type;

    /// Is this a pipeline library?
    /// These are non-executable, but can be used ot create other pipelines
    bool isLibrary{false};

    /// Replaced pipeline object, fx. instrumented version
    std::atomic<VkPipeline> hotSwapObject{VK_NULL_HANDLE};

    /// Layout for this pipeline
    PipelineLayoutState* layout{nullptr};

    /// All shader modules used to compile this pipeline state
    std::vector<ShaderModuleState*> ownedShaderModules;

    /// All referenced shader modules in this state,
    /// including both owned and those from libraries
    std::vector<ShaderModuleState*> referencedShaderModules;

    /// Referenced shader modules
    std::vector<ShaderModuleInstrumentationKey> referencedInstrumentationKeys;

    /// Referenced shader modules
    std::vector<uint64_t> libraryInstrumentationKeys;

    /// Referenced shader modules
    std::vector<PipelineState*> pipelineLibraries;

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

struct RaytracingPipelineState : public PipelineState {
    /// Placeholder
};
