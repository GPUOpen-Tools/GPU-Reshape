#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <atomic>

enum class PipelineType {
    Graphics,
    Compute
};

struct PipelineState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineState();

    /// User pipeline
    ///  ! May be nullptr if the top pipeline has been destroyed
    VkPipeline object{VK_NULL_HANDLE};

    /// Type of the pipeline
    PipelineType type;

    /// Replaced pipeline object, fx. instrumented version
    std::atomic<VkPipeline> hotSwapObject{VK_NULL_HANDLE};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;
};

struct GraphicsPipelineState : public PipelineState {
    /// Recreation info
    VkGraphicsPipelineCreateInfoDeepCopy createInfoDeepCopy;
};

struct ComputePipelineState : public PipelineState {
    /// Recreation info
    VkComputePipelineCreateInfoDeepCopy createInfoDeepCopy;
};

/*
    2.1. Batched work, fx, recompile all pipelines, # / 30 per-worker

    Who gets recompiled when?
    1. Some system knows about all the pipelines
    1.2. Instrumentation Constraints
    1.2.1. List of flags for how strict the instrumentation is.
           fx. ok to record when pipeline is recreating, don't wait

    1.3 What pipelines to instrument
    1.3.1 Could be global, but feel that is lazy.
    1.3.2 Individual? Better control, perfect control really. (edit: DO BOTH, prevents race conditions, simplifies logic)
    1.3.2.1 Latency, with 50k pipelines that's 200kb max (4b), couple of milliseconds to really transfer that across network
    1.3.2.2 Small latency on new creation or uses, maybe that's ok? Will need recompilation anyway...

    1.4 What are we instrumenting? Pipelines or shaders? Both?
    1.4.1 Each keeps their own instrumentation set, super positioned

    1.5 Instrumentation specialization, fx. debugging instructions
    1.5.1 What line are we looking at
    1.5.2 Instrumentation specialization per message

        GlobalInstrumentationMessage
            FeatureSet = 0000000000

            Specialization

        ShaderInstrumentationMessage
            FeatureSet = 0000001000

            Specialization (sub stream, literally an array of uint8_t)
                DebugSpecialization
                    ID = H("DebugSpecialization")
                    InstructionID = 2931

        PipelineInstrumentationMessage
            PipelineID = 55

            FeatureSet = 0000000000

            Specialization
*/