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

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/ShaderProgram/ShaderProgram.h>
#include <Backend/IL/Emitter.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>

// Forward declarations
class SignalShaderProgram;
class IShaderSGUIDHost;
class IScheduler;

class LoopFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(LoopFeature);

    /// Constructor
    LoopFeature();

    /// Destructor
    ~LoopFeature();

    /// IFeature
    bool Install() override;
    bool PostInstall() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
    void Inject(IL::Program &program, const MessageStreamView<> &specialization) override;

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
    using LoopCounterMap = std::map<IL::ID, IL::ID>;

    /// Inject all function wide loop counters
    /// @param program source program
    /// @param map destination map
    void InjectLoopCounters(IL::Program &program, LoopCounterMap& map);

    /// Increment the function local counter
    /// @param emitter target emitter
    /// @param function function to increment
    /// @param map allocated counters
    /// @return current iteration value
    IL::ID GetAndIncrementCounter(IL::Emitter<>& emitter, IL::Function *function, LoopCounterMap &map);

private:
    /// Feature hooks
    void OnOpen(CommandContext *context);
    void OnPostSubmit(const CommandContextHandle* contextHandles, uint32_t count);
    void OnJoin(CommandContextHandle contextHandle);

private:
    /// Max number of live submissions
    static constexpr uint32_t kMaxTrackedSubmissions = 16384;

    /// Cyclic event counter
    uint32_t submissionAllocationCounter{0};

    /// Allocate a new termination id
    uint32_t AllocateTerminationIDNoLock();

private:
    struct CommandContextState {
        /// Time point of the submission
        std::chrono::high_resolution_clock::time_point submissionStamp;

        /// Is this state pending?
        bool pending{false};

        /// Is this state pending?
        bool terminated{false};

        /// Allocated termination id
        uint32_t terminationID{0};
    };

    /// Async heart beat thread
    std::thread heartBeatThread;

    /// Async exit flag
    std::atomic<bool> heartBeatExitFlag{false};

    /// All known context states
    std::unordered_map<CommandContextHandle, CommandContextState> contextStates;

    /// Internal worker
    void HeartBeatThreadWorker();

private:
    /// Shader data
    ShaderDataID terminationBufferID{InvalidShaderDataID};
    ShaderDataID terminationAllocationID{InvalidShaderDataID};

    /// Permanently mapped termination data
    uint32_t* terminationData{nullptr};

private:
    /// Shared mutex
    std::mutex mutex;

    /// Components
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost>  shaderDataHost{nullptr};
    ComRef<IScheduler>       scheduler{nullptr};

    /// Signal program
    ComRef<SignalShaderProgram> signalShaderProgram;

    /// Allocated program ID
    ShaderProgramID signalShaderProgramID{InvalidShaderProgramID};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};