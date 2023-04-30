#pragma once

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/IL/ResourceTokenType.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>

// Forward declarations
class IShaderSGUIDHost;

class DescriptorFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(DescriptorFeature);

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
    /// Inject instrumentation for a given resource
    /// \param program source program
    /// \param function source function
    /// \param it source instruction reference from which instrumentation occurs, potentially safe-guarded
    /// \param resource resource to validate
    /// \param compileTypeLiteral expected compile type value
    /// \param isSafeGuarded to safeguard, or not to safeguard
    /// \return moved iterator
    IL::BasicBlock::Iterator InjectForResource(IL::Program& program, IL::Function& function, IL::BasicBlock::Iterator it, IL::ID resource, Backend::IL::ResourceTokenType compileTypeLiteral, bool isSafeGuarded);

private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};