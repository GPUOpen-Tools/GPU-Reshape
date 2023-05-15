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

// Forward declarations
class IShaderSGUIDHost;

class LoopFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(LoopFeature);

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
    /// Max number of live submissions
    static constexpr uint32_t kMaxTrackedSubmissions = 16384;

private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shader data
    ShaderDataID terminationBufferID{InvalidShaderDataID};

    /// Cyclic event counter
    std::atomic<uint32_t> submissionAllocationCounter{0};

    /// Shared stream
    MessageStream stream;
};