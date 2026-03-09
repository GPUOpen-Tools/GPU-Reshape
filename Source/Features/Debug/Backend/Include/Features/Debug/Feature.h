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
#include <Features/Debug/WatchpointHeader.h>

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
#include <Backend/Device/DeviceCapabilityTable.h>
#include <Backend/IL/Debug/DebugStack.h>

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
class IDeviceProperties;
class IScheduler;
class IShaderSGUIDHost;
class LooseAcquisitionProgram;
class ResetHeaderProgram;
class FinalizePredicateProgram;
class SetupPredicateProgram;
class IndirectCopyProgram;
class SetupIndirectCopyProgram;
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

    /// Invoked on watchpoint acquisition
    /// @param acqMessage the message
    /// @param builder immediate builder
    void OnWatchpointAcquired(const WatchpointAcquisitionMessage* acqMessage, CommandBuilder& builder);

private:
    /// Interrupt a visitation context
    /// @param it instruction to interrupt
    /// @param interruptBlock the finalized interrupt block
    /// @return the new iteration point
    IL::BasicBlock::Iterator SplitInterruptBlock(const IL::VisitContext& context, IL::BasicBlock::Iterator it, IL::BasicBlock* interruptBlock);

    /// Inject a watchpoint
    /// @param it instruction to debug
    /// @param watchpoint watchpoint to be added
    /// @return next iterator
    IL::BasicBlock::Iterator InjectWatchpoint(const IL::VisitContext& context, IL::BasicBlock::Iterator it, const DebugWatchpointMessage& watchpoint);

private:
    struct Watchpoint {
        /// Monotic id of this watchpoint
        uint32_t uid = 0;

        /// Host layout, determined at compile time
        std::unordered_map<uint32_t, WatchpointDataHostLayout> hostLayoutMap;

        /// Registered capture mode
        WatchpointCaptureMode captureMode{};

        /// Allocated stream size
        uint64_t streamSize = 0;

        /// Gpu produced hash
        uint32_t pendingCollectionHash = 0;
        
        /// Gpu produced counter
        uint32_t pendingAcqDynamicCounter = 0;

        /// Is this watchpoint pending collection?
        bool pendingCollection = false;

        /// Do we have a pending header upload?
        bool pendingTransferHeader = true;

        /// Do we have a pending header reset?
        bool pendingHeaderReset = false;

        /// The templated header for blitting
        WatchpointHeader header{};

        /// Underlying allocation
        BuddyAllocation streamAllocation;

        /// Streaming buffer
        ShaderDataID hostStreamingBuffer = InvalidShaderDataID;
    };

    struct WatchpointData {
        /// Flags for this watchpoint
        WatchpointFlag flags{WatchpointFlag::None};

        /// Optional, marker hash
        uint32_t markerHash32{0};

        /// Current instrumentation hash
        uint32_t shaderInstrumentationHash32{0};

        /// Intermediate host layout data
        WatchpointDataHostLayout hostLayout{};
        
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

        /// Shared allocator
        SmallArena arena;

        /// Reconstructed stack
        IL::DebugStack debugStack;
        
        /// Variable lookup
        std::unordered_map<uint64_t, const IL::DebugVariable*> variables;
        
        /// Chosen variable
        uint64_t variableHash = UINT64_MAX;
        
        /// Chosen value within the variable
        uint32_t valueId = UINT32_MAX;
    };

    struct PendingDestruction {
        /// Allocation to be released
        BuddyAllocation allocation;

        /// Streaming b uffer to be released
        ShaderDataID hostStreamingBuffer = InvalidShaderDataID;

        /// The last commit that used the allocations
        uint64_t lastCommit = 0;
    };

    /// Find a watchpoint from uid
    Watchpoint* FindWatchpointNoLock(uint32_t uid);

    /// Apply program wide flags
    /// @param context parent context
    /// @param watchpointData device watchpoint data
    void ApplyWatchpointFlagsToProgram(const IL::VisitContext& context, const WatchpointData& watchpointData);

    /// Get watchpoint device data
    /// @param emitter target emitter
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    void GetWatchpoint(IL::Emitter<>& emitter, Watchpoint* watchpoint, WatchpointData& watchpointData);

    /// Get a watchpoint from its message
    /// @param emitter target emitter
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    void GetWatchpoint(IL::Emitter<>& emitter, DebugWatchpointMessage watchpoint, WatchpointData& watchpointData);

    /// Store all value dwords of a watchpoint
    /// @param context parent context
    /// @param emitter target emitter
    /// @param value value, potentially structured, to be stored
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    void StoreWatchpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Watchpoint* watchpoint, WatchpointData& watchpointData);

    /// Get the raw (i.e., not reconstructed) debug value
    /// @param context parent context
    /// @param instr debug instruction
    /// @return invalid if failed
    IL::ID GetInstructionRawDebugValue(const IL::VisitContext &context, const IL::Instruction* instr);
    
    /// Get the debug type for an instruction
    /// @param context parent context
    /// @param instr debug instruction
    /// @return invalid if failed
    const Backend::IL::Type* GetInstructionDebugType(const IL::VisitContext &context, const IL::Instruction* instr, const DebugWatchpointMessage& watchpointMessage, WatchpointData& watchpointData, Watchpoint* watchpoint);
    
    /// Get the debug value for an instruction
    /// @param context parent context
    /// @param instr debug instruction
    /// @param insertIt the insertion iterator for reconstruction
    /// @return invalid if failed
    IL::ID GetInstructionDebugValue(const IL::VisitContext &context, const IL::Instruction* instr, WatchpointData& watchpointData, IL::BasicBlock::Iterator& insertIt);
    
    /// Try to get the texel format of a watchpoint
    /// @return true if a format is appropriate, over structured data
    bool GetWatchpointFormat(WatchpointData& watchpointData);

    /// Check if a kernel type is supported
    /// @param kernelType type to check
    /// @param watchpoint active watchpoint
    /// @return supported
    bool SupportsKernelType(IL::KernelType kernelType, Watchpoint* watchpoint);
    
    /// Try to get the watchpoint data host layout, fails in case it's not a valid watchpoint
    /// @param context parent context
    /// @param instr exporting instruction
    /// @param value value to check for
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    /// @return false if failed
    bool GetWatchpointDataHostLayout(const IL::VisitContext &context, const IL::Instruction* instr, const Backend::IL::Type* valueType, Watchpoint *watchpoint, WatchpointData& watchpointData);

    /// Store all exported watchpoint data
    /// @param context parent context
    /// @param emitter target emitter
    /// @param instr exporting instruction
    /// @param value value to check for
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    void StoreWatchpointData(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Watchpoint* watchpoint, WatchpointData& watchpointData);

    /// Get first event ordering
    /// @param context parent context
    /// @param emitter target emitter
    /// @param execution the current execution info
    /// @param watchpointHeader the watchpoint header state
    /// @param watchpoint host watchpoint data
    /// @param watchpointData device watchpoint data
    void GetWatchpointOrderingFirstEvent(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ShaderStruct<ExecutionInfo>& execution, IL::ShaderBufferStruct<WatchpointHeader>& watchpointHeader, Watchpoint *watchpoint, WatchpointData& watchpointData);

    /// Acquire a first event watchpoint
    /// @param it instruction being instrumented
    /// @param watchpointBlock the watchpoint interrupt block
    /// @param watchpoint watchpoint data
    /// @param watchpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireWatchpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *watchpointBlock, Watchpoint* watchpoint, WatchpointData& watchpointData);
    
    /// Acquire and allocate first event ordering
    /// @param it instruction being instrumented
    /// @param watchpointBlock the watchpoint interrupt block
    /// @param watchpoint watchpoint data
    /// @param watchpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireAndAllocateWatchpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *watchpointBlock, Watchpoint* watchpoint, WatchpointData& watchpointData);
    
    /// Acquire and allocate all event ordering
    /// @param it instruction being instrumented
    /// @param watchpointBlock the watchpoint interrupt block
    /// @param watchpoint watchpoint data
    /// @param watchpointData
    /// @return next instruction iterator
    IL::BasicBlock* AcquireAndAllocateWatchpointAllEvents(const IL::VisitContext &context, const IL::BasicBlock::Iterator &insertIt, IL::BasicBlock *watchpointBlock, Watchpoint* watchpoint, WatchpointData& watchpointData);

    /// Check if we can collect any watchpoint data
    /// @param watchpoint target watchpoint
    bool CanCollectWatchpoint(const Watchpoint& watchpoint);

    /// Get the current instrumentation hash used for host layout matching
    /// @param watchpoint target watchpoint
    /// @param header mapped header
    uint32_t GetWatchpointInstrumentationHash(const Watchpoint& watchpoint, const WatchpointHeader* header);

    /// Check if a watchpoint header has any valid data for collection
    /// @param watchpoint target watchpoint
    /// @param header mapped header
    bool HasWatchpointStreambackData(const Watchpoint& watchpoint, const WatchpointHeader* header);

    /// Get the actual watchpoint streaming size
    /// @param watchpoint target watchpoint
    /// @param header mapped header
    /// @return stream byte size
    uint64_t GetWatchpointStreamRequestSize(const Watchpoint& watchpoint, const WatchpointHeader* header, const WatchpointDataHostLayout& hostLayout);

private:
    /// Create the payload for a given watchpoint
    /// and update the templated header
    /// @param watchpoint watchpoint to update
    void CreateAndUpdatePayload(Watchpoint& watchpoint);
    
private:
    /// All watchpoints
    std::vector<Watchpoint> watchpoints;

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
    ComRef<IDeviceProperties> deviceProperties;
    
    /// Device table
    DeviceCapabilityTable deviceCapabilityTable;

    /// All device states
    DeviceStateRef<DeviceStatePooling> poolingState;

    /// Stored as naked pointer due to reference counting
    IBridge* bridge{nullptr};

    /// Shader data
    ShaderDataID streamBufferID{InvalidShaderDataID};

    /// Programs
    ComRef<SetupPredicateProgram> setupPredicateProgram;
    ComRef<FinalizePredicateProgram> finalizePredicateProgram;
    ComRef<SetupIndirectCopyProgram> setupIndirectCopyProgram;
    ComRef<IndirectCopyProgram> indirectCopyProgram;
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
