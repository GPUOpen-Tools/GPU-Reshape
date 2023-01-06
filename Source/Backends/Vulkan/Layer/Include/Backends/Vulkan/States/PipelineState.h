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
