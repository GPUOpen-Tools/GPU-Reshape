#pragma once

// Backend
#include "ShaderData.h"
#include "ShaderDataBufferInfo.h"
#include "ShaderDataEventInfo.h"
#include "ShaderDataDescriptorInfo.h"
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

    /// Create a new descriptor data
    /// \param info descriptor data information
    /// \return invalid if failed
    virtual ShaderDataID CreateDescriptorData(const ShaderDataDescriptorInfo& info) = 0;

    /// Map a buffer
    /// \param rid resource id
    /// \return mapped buffer
    virtual void* Map(ShaderDataID rid) = 0;

    /// Flush a mapped range
    /// \param rid resource id
    /// \param offset byte offset
    /// \param length byte length from offset
    virtual void FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) = 0;

    /// Destroy an allocation
    /// \param rid allocation identifier
    virtual void Destroy(ShaderDataID rid) = 0;

    /// Enumerate all created data
    /// \param count if [out] is null, filled with the number of resources
    /// \param out if not null, filled with all resources up to [count]
    virtual void Enumerate(uint32_t* count, ShaderDataInfo* out, ShaderDataTypeSet mask) = 0;
};
