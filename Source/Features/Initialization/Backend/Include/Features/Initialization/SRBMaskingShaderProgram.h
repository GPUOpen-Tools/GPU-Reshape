#pragma once

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/ShaderProgram/IShaderProgram.h>
#include <Backend/ShaderData/ShaderData.h>

class SRBMaskingShaderProgram final : public IShaderProgram {
public:
    /// Constructor
    /// \param initializationMaskBufferID
    SRBMaskingShaderProgram(ShaderDataID initializationMaskBufferID);

    /// Install the masking program
    /// \return
    bool Install();

    /// IShaderProgram
    void Inject(IL::Program &program) override;

    /// Interface querying
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case IShaderProgram::kID:
                return static_cast<IShaderProgram*>(this);
        }

        return nullptr;
    }

    /// Get the mask ID
    ShaderDataID GetMaskEventID() const {
        return maskEventID;
    }

    /// Get the PUID ID
    ShaderDataID GetPUIDEventID() const {
        return puidEventID;
    }

private:
    /// Shared data host
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Shader data
    ShaderDataID initializationMaskBufferID{InvalidShaderDataID};
    ShaderDataID maskEventID{InvalidShaderDataID};
    ShaderDataID puidEventID{InvalidShaderDataID};
};
