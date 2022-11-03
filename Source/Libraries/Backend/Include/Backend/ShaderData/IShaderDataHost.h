#pragma once

#include "ShaderData.h"
#include "ShaderDataBufferInfo.h"
#include "ShaderDataEventInfo.h"
#include "ShaderDataInfo.h"

// Common
#include <Common/IComponent.h>

class IShaderDataHost : public TComponent<IShaderDataHost> {
public:
    COMPONENT(IShaderDataHost);

    /// Create a new buffer
    /// \param info buffer information
    /// \return invalid if failed
    virtual ShaderDataID CreateBuffer(const ShaderDataBufferInfo& info) = 0;

    /// Create a new event data
    /// \param info buffer information
    /// \return invalid if failed
    virtual ShaderDataID CreateEventData(const ShaderDataEventInfo& info) = 0;

    /// Destroy an allocation
    /// \param rid allocation identifier
    virtual void Destroy(ShaderDataID rid) = 0;

    /// Enumerate all created data
    /// \param count if [out] is null, filled with the number of resources
    /// \param out if not null, filled with all resources up to [count]
    virtual void Enumerate(uint32_t* count, ShaderDataInfo* out, ShaderDataTypeSet mask) = 0;
};
