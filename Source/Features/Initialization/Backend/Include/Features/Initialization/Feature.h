#pragma once

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include "Backend/ShaderProgram/ShaderProgram.h"

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>
#include <mutex>
#include <unordered_map>

// Forward declarations
class IShaderSGUIDHost;
class SRBMaskingShaderProgram;

class InitializationFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(InitializationFeature);

    /// IFeature
    bool Install() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
    void Inject(IL::Program &program) override;

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
    /// Hooks
    void OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest);
    void OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest);
    void OnClearResource(CommandContext* context, const ResourceInfo& buffer);
    void OnWriteResource(CommandContext* context, const ResourceInfo& buffer);
    void OnBeginRenderPass(CommandContext* context, const RenderPassInfo& passInfo);

private:
    /// Mark a resource SRB
    /// \param context destination context
    /// \param puid physical resource uid
    /// \param srb resource mask
    void MaskResourceSRB(CommandContext* context, uint64_t puid, uint32_t srb);

private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Shader data
    ShaderDataID initializationMaskBufferID{InvalidShaderDataID};

    /// Masking program
    ComRef<SRBMaskingShaderProgram> srbMaskingShaderProgram;

    /// Allocated program ID
    ShaderProgramID srbMaskingShaderProgramID{InvalidShaderProgramID};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;

private:
    /// Shared lock
    std::mutex mutex;

    /// Current initialization mask
    std::unordered_map<uint64_t, uint32_t> puidSRBInitializationMap;
};