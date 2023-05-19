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

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>
#include <unordered_map>
#include <chrono>

// Forward declarations
class IShaderSGUIDHost;

class LoopFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(LoopFeature);

    /// Constructor
    LoopFeature();

    /// Destructor
    ~LoopFeature();

    /// IFeature
    bool Install() override;
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
    /// Feature hooks
    void OnOpen(CommandContext *context);
    void OnSubmit(CommandContextHandle contextHandle);
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

    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};