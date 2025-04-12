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

// Debug
#include <Features/Debug/BreakpointType.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/Scheduler/SchedulerPrimitive.h>

// Schemas
#include <Schemas/Features/DebugConfig.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Addressing
#include <Addressing/TileResidencyAllocator.h>

// Message
#include <Message/MessageStream.h>

// Common
#include <Common/ComRef.h>
#include <Common/Allocator/BuddyAllocator.h>

// Forward declarations
class IScheduler;
class IShaderSGUIDHost;

class DebugFeature final : public IFeature, public IShaderFeature, public IBridgeListener {
public:
    COMPONENT(DebugFeature);

    /// IFeature
    bool Install() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
    void Inject(IL::Program &program, const MessageStreamView<> &specialization) override;

    /// IBridgeListener
    void Handle(const MessageStream *streams, uint32_t count) override;

    /// Interface querying
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case IFeature::kID:
                return static_cast<IFeature*>(this);
            case IShaderFeature::kID:
                return static_cast<IShaderFeature*>(this);
        }

        return nullptr;
    }

private:
    /// Hook tables
    void OnSubmitBatchBegin(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount);
    void OnSyncPoint();

private:
    /// Get the static ordering for a value
    /// @param it value to get ordering for
    /// @return ordering
    IL::ID GetStaticOrderingFor(const IL::VisitContext &context, IL::Emitter<>& emitter, const IL::Instruction *it);

    /// Inject a breakpoint
    /// @param it instruction to debug
    /// @param breakpoint breakpoint to be added
    /// @return next iterator
    IL::BasicBlock::Iterator InjectBreakpoint(const IL::VisitContext& context, const IL::BasicBlock::Iterator& it, DebugBreakpointMessage breakpoint);

private:
    struct Breakpoint {
        /// Monotic id of this breakpoint
        uint32_t uid = 0;

        /// Type of this breakpoint
        BreakpointType type{};

        /// Allocated stream size
        uint64_t streamSize = 0;

        /// Underlying allocation
        BuddyAllocation allocation;

        /// Streaming buffer
        ShaderDataID hostStreamingBuffer = InvalidShaderDataID;

        /// Type payload
        union {
            struct {
                uint32_t width;
                uint32_t height;
            } image;
        } payload;
    };

    /// All breakpoints
    std::vector<Breakpoint> breakpoints;

    /// Debug memory allocator
    BuddyAllocator buddyAllocator;

    /// Debug memory tile allocator
    TileResidencyAllocator tileResidencyAllocator;
    
private:
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
    using Duration  = std::chrono::milliseconds;

    /// Throttling configurables
    static constexpr double   kMinDurationBeforeThrottle = 30;
    static constexpr double   kMaxDurationBeforeDrop     = 2000;
    static constexpr uint32_t kRequestThrottleWindow     = 8;

    struct RequestController {
        /// Last time the request was satisfied
        TimePoint lastStreamBufferTime = TimePoint{};

        /// Current streaming interval
        double streamBufferInterval = 100;

        /// Current request index (local)
        uint32_t requestIndex = 0;

        /// Current remote request index
        uint32_t remoteRequestIndex = 0;
    };

    /// The shared throttling controller
    RequestController defaultController;

    /// Throttle against a controller
    /// @param controller controller to be throttled
    /// @return false if throttled
    bool ThrottleController(RequestController& controller);

private:
    /// Monotonically incremented primitive counter
    uint64_t exclusiveTransferPrimitiveMonotonicCounter{0};

    /// Primitive used for all transfer synchronization
    SchedulerPrimitiveID exclusiveTransferPrimitiveID{InvalidSchedulerPrimitiveID};
    
private:
    /// Shared lock
    std::mutex mutex;
    
    /// Shader SGUID
    ComRef<IShaderSGUIDHost> sguidHost;
    ComRef<IShaderDataHost>  shaderDataHost;
    ComRef<IScheduler>       scheduler;

    /// Shader data
    ShaderDataID streamBufferID{InvalidShaderDataID};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};
