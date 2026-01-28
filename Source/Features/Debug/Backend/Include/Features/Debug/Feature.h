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
#include <Features/Debug/BreakpointHeader.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/Scheduler/SchedulerPrimitive.h>
#include <Backend/Feature/ContextLifetimeQueue.h>
#include <Backend/ShaderProgram/ShaderProgram.h>
#include <Backend/Device/DeviceStateRef.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Metadata/KernelMetadata.h>

// Schemas
#include <Schemas/Features/DebugConfig.h>
#include <Schemas/Features/Debug.h>

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
class IBridge;
class IScheduler;
class IShaderSGUIDHost;
class LooseAcquisitionProgram;
class ResetHeaderProgram;
struct CommandBuilder;

namespace IL {
    class IDebugEmitter;
}

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
    /// Create all programs
    bool CreatePrograms();

private:
    /// Hook tables
    void OnPreSubmit(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount);
    void OnJoin(CommandContextHandle contextHandle);
    void OnSyncPoint();

    /// Invoked on breakpoint acquisition
    /// @param acqMessage the message
    /// @param builder immediate builder
    void OnBreakpointAcquired(const BreakpointAcquisitionMessage* acqMessage, CommandBuilder& builder);

private:
    /// Interrupt a visitation context
    /// @param it instruction to interrupt
    /// @param interruptBlock the finalized interrupt block
    /// @return the new iteration point
    IL::BasicBlock::Iterator SplitInterruptBlock(const IL::VisitContext& context, IL::BasicBlock::Iterator it, IL::BasicBlock* interruptBlock);

    /// Inject a breakpoint
    /// @param it instruction to debug
    /// @param breakpoint breakpoint to be added
    /// @return next iterator
    IL::BasicBlock::Iterator InjectBreakpoint(const IL::VisitContext& context, IL::BasicBlock::Iterator it, const DebugBreakpointMessage& breakpoint);

private:
    struct Breakpoint {
        /// Monotic id of this breakpoint
        uint32_t uid = 0;

        /// Host layout, determined at compile time
        std::unordered_map<uint32_t, BreakpointDataHostLayout> hostLayoutMap;

        /// Registered capture mode
        BreakpointCaptureMode captureMode{};

        /// Allocated stream size
        uint64_t streamSize = 0;

        /// Gpu produced hash
        uint32_t pendingCollectionHash = 0;
        
        /// Gpu produced counter
        uint32_t pendingAcqDynamicCounter = 0;

        /// Is this breakpoint pending collection?
        bool pendingCollection = false;

        /// Do we have a pending header upload?
        bool pendingTransferHeader = true;

        /// Do we have a pending header reset?
        bool pendingHeaderReset = false;

        /// The templated header for blitting
        BreakpointHeader header{};

        /// Underlying allocation
        BuddyAllocation streamAllocation;

        /// Streaming buffer
        ShaderDataID hostStreamingBuffer = InvalidShaderDataID;
    };

    struct BreakpointData {
        /// Flags for this breakpoint
        BreakpointFlag flags{BreakpointFlag::None};

        /// Optional, marker hash
        uint32_t markerHash32{0};

        /// Current instrumentation hash
        uint32_t shaderInstrumentationHash32{0};

        /// Intermediate host layout data
        BreakpointDataHostLayout hostLayout{};
        
        /// The dynamically assigned ordering type
        IL::ID orderType{IL::InvalidID};

        /// The calculated static order
        IL::ID staticOrder{IL::InvalidID};

        /// The assigned export order
        IL::ID exportOrder{IL::InvalidID};

        /// Capture mode data
        union {
            struct {
                /// The statically computed ordering dimensions
                IL::ID staticOrderWidth;
                IL::ID staticOrderHeight;
                IL::ID staticOrderDepth;
                
                /// The total number of streamed dwords
                IL::ID dwordStreamCount;

                /// Is this a dynamic export?
                IL::ID isDynamic;
            } firstEvent;
        };

        /// The header offset
        IL::ID headerOffset{IL::InvalidID};

        /// The data pyaload offset
        IL::ID payloadOffset{IL::InvalidID};
        
        /// The payload data dword offset
        IL::ID payloadDataOffset{IL::InvalidID};
        
        /// Chosen variable
        uint32_t variableHandle = UINT32_MAX;
    };

    struct PendingDestruction {
        /// Allocation to be released
        BuddyAllocation allocation;

        /// Streaming b uffer to be released
        ShaderDataID hostStreamingBuffer = InvalidShaderDataID;

        /// The last commit that used the allocations
        uint64_t lastCommit = 0;
    };

    /// Find a breakpoint from uid
    Breakpoint* FindBreakpointNoLock(uint32_t uid);

    /// Apply program wide flags
    /// @param context parent context
    /// @param breakpointData device breakpoint data
    void ApplyBreakpointFlagsToProgram(const IL::VisitContext& context, const BreakpointData& breakpointData);

    /// Get breakpoint device data
    /// @param emitter target emitter
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    void GetBreakpoint(IL::Emitter<>& emitter, Breakpoint* breakpoint, BreakpointData& breakpointData);

    /// Get a breakpoint from its message
    /// @param emitter target emitter
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    void GetBreakpoint(IL::Emitter<>& emitter, DebugBreakpointMessage breakpoint, BreakpointData& breakpointData);

    /// Store all value dwords of a breakpoint
    /// @param context parent context
    /// @param emitter target emitter
    /// @param value value, potentially structured, to be stored
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    void StoreBreakpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Breakpoint* breakpoint, BreakpointData& breakpointData);

    /// Get the raw (i.e., not reconstructed) debug value
    /// @param context parent context
    /// @param instr debug instruction
    /// @return invalid if failed
    IL::ID GetInstructionRawDebugValue(const IL::VisitContext &context, const IL::Instruction* instr);
    
    /// Get the debug type for an instruction
    /// @param context parent context
    /// @param instr debug instruction
    /// @return invalid if failed
    const Backend::IL::Type* GetInstructionDebugType(const IL::VisitContext &context, const IL::Instruction* instr, const DebugBreakpointMessage& breakpointMessage, BreakpointData& breakpointData, Breakpoint* breakpoint);
    
    /// Get the debug value for an instruction
    /// @param context parent context
    /// @param instr debug instruction
    /// @param insertIt the insertion iterator for reconstruction
    /// @return invalid if failed
    IL::ID GetInstructionDebugValue(const IL::VisitContext &context, const IL::Instruction* instr, BreakpointData& breakpointData, IL::BasicBlock::Iterator& insertIt);
    
    /// Try to get the texel format of a breakpoint
    /// @return true if a format is appropriate, over structured data
    bool GetBreakpointFormat(BreakpointData& breakpointData);

    /// Check if a kernel type is supported
    /// @param kernelType type to check
    /// @param breakpoint active breakpoint
    /// @return supported
    bool SupportsKernelType(IL::KernelType kernelType, Breakpoint* breakpoint);
    
    /// Try to get the breakpoint data host layout, fails in case it's not a valid breakpoint
    /// @param context parent context
    /// @param instr exporting instruction
    /// @param value value to check for
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    /// @return false if failed
    bool GetBreakpointDataHostLayout(const IL::VisitContext &context, const IL::Instruction* instr, const Backend::IL::Type* valueType, Breakpoint *breakpoint, BreakpointData& breakpointData);

    /// Store all exported breakpoint data
    /// @param context parent context
    /// @param emitter target emitter
    /// @param instr exporting instruction
    /// @param value value to check for
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    void StoreBreakpointData(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Breakpoint* breakpoint, BreakpointData& breakpointData);

    /// Get first event ordering
    /// @param context parent context
    /// @param emitter target emitter
    /// @param execution the current execution info
    /// @param breakpointHeader the breakpoint header state
    /// @param breakpoint host breakpoint data
    /// @param breakpointData device breakpoint data
    void GetBreakpointOrderingFirstEvent(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ShaderStruct<ExecutionInfo>& execution, IL::ShaderBufferStruct<BreakpointHeader>& breakpointHeader, Breakpoint *breakpoint, BreakpointData& breakpointData);

    /// Acquire a first event breakpoint
    /// @param it instruction being instrumented
    /// @param breakpointBlock the breakpoint interrupt block
    /// @param breakpoint breakpoint data
    /// @param breakpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireBreakpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *breakpointBlock, Breakpoint* breakpoint, BreakpointData& breakpointData);
    
    /// Acquire and allocate first event ordering
    /// @param it instruction being instrumented
    /// @param breakpointBlock the breakpoint interrupt block
    /// @param breakpoint breakpoint data
    /// @param breakpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireAndAllocateBreakpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *breakpointBlock, Breakpoint* breakpoint, BreakpointData& breakpointData);
    
    /// Acquire and allocate all event ordering
    /// @param it instruction being instrumented
    /// @param breakpointBlock the breakpoint interrupt block
    /// @param breakpoint breakpoint data
    /// @param breakpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireAndAllocateBreakpointAllEvents(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *breakpointBlock, Breakpoint* breakpoint, BreakpointData& breakpointData);

    /// Check if we can collect any breakpoint data
    /// @param breakpoint target breakpoint
    bool CanCollectBreakpoint(const Breakpoint& breakpoint);

    /// Get the current instrumentation hash used for host layout matching
    /// @param breakpoint target breakpoint
    /// @param header mapped header
    uint32_t GetBreakpointInstrumentationHash(const Breakpoint& breakpoint, const BreakpointHeader* header);

    /// Check if a breakpoint header has any valid data for collection
    /// @param breakpoint target breakpoint
    /// @param header mapped header
    bool HasBreakpointStreambackData(const Breakpoint& breakpoint, const BreakpointHeader* header);

    /// Get the actual breakpoint streaming size
    /// @param breakpoint target breakpoint
    /// @param header mapped header
    /// @return stream byte size
    uint64_t GetBreakpointStreamRequestSize(const Breakpoint& breakpoint, const BreakpointHeader* header, const BreakpointDataHostLayout& hostLayout);

private:
    /// Create the payload for a given breakpoint
    /// and update the templated header
    /// @param breakpoint breakpoint to update
    void CreateAndUpdatePayload(Breakpoint& breakpoint);
    
private:
    /// All breakpoints
    std::vector<Breakpoint> breakpoints;

    /// Debug memory allocator
    BuddyAllocator buddyAllocator;

    /// Debug memory tile allocator
    TileResidencyAllocator tileResidencyAllocator;

    /// All pending destructions
    std::vector<PendingDestruction> allocationDestructionQueue;
    
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

    /// Lifetime queue for safe destruction
    ContextLifetimeQueue contextLifetimeQueue;
    
private:
    /// Shared lock
    std::mutex mutex;
    
    /// Shared components
    ComRef<IShaderSGUIDHost>  sguidHost;
    ComRef<IShaderDataHost>   shaderDataHost;
    ComRef<IScheduler>        scheduler;
    ComRef<IDeviceStateVote>  stateVote;
    ComRef<IL::IDebugEmitter> debugEmitter;

    /// All device states
    DeviceStateRef<DeviceStatePooling> poolingState;

    /// Stored as naked pointer due to reference counting
    IBridge* bridge{nullptr};

    /// Shader data
    ShaderDataID streamBufferID{InvalidShaderDataID};

    /// Programs
    ComRef<LooseAcquisitionProgram> looseAcquisitionProgram;
    ComRef<ResetHeaderProgram> resetHeaderProgram;

    /// Program ids
    ShaderProgramID looseAcquisitionProgramID{InvalidShaderProgramID};
    ShaderProgramID resetHeaderProgramID{InvalidShaderProgramID};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};
