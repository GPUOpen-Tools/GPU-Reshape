#pragma once

#include "ShaderResource.h"
#include "ShaderBufferInfo.h"
#include "ShaderResourceInfo.h"

// Common
#include <Common/IComponent.h>

class IShaderResourceHost : public TComponent<IShaderResourceHost> {
public:
    COMPONENT(IShaderResourceHost);

    /// Create a new buffer
    /// \param info buffer information
    /// \return invalid if failed
    virtual ShaderResourceID CreateBuffer(const ShaderBufferInfo& info) = 0;

    /// Destroy a buffer
    /// \param rid allocation identifier
    virtual void DestroyBuffer(ShaderResourceID rid) = 0;

    /// Enumerate all created resources
    /// \param count if [out] is null, filled with the number of resources
    /// \param out if not null, filled with all resources up to [count]
    virtual void Enumerate(uint32_t* count, ShaderResourceInfo* out) = 0;
};
