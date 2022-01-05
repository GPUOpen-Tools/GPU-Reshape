#pragma once

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>

// Message
#include <Message/MessageStream.h>

// Forward declarations
struct IShaderSGUIDHost;

class ResourceBoundsFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(ResourceBoundsFeature);

    /// IFeature
    bool Install() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
    void Inject(IL::Program &program) override;

    /// Interface querying
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IFeature::kID:
                return static_cast<IFeature*>(this);
            case IShaderFeature::kID:
                return static_cast<IShaderFeature*>(this);
        }

        return nullptr;
    }

private:
    /// Shader SGUID
    IShaderSGUIDHost* sguidHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};