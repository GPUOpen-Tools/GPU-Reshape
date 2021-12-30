#pragma once

#include "ShaderExport.h"
#include "ShaderExportTypeInfo.h"

// Common
#include <Common/IComponent.h>

class IShaderExportHost : public IComponent {
public:
    COMPONENT(IShaderExportHost);

    /// Allocate a shader export
    /// \param typeInfo the type information for the export
    /// \return the allocation identifier
    virtual ShaderExportID Allocate(const ShaderExportTypeInfo& typeInfo) = 0;

    /// Get the type info of an export
    /// \param id id of the export
    /// \return the type info
    virtual ShaderExportTypeInfo GetTypeInfo(ShaderExportID id) = 0;

    /// Enumerate all exports
    /// \param count if [out] is nullptr, filled with the number of exports
    /// \param out optional, if valid is filled with [count] exports
    virtual void Enumerate(uint32_t* count, ShaderExportID* out) = 0;

    /// Get the shader export bound
    /// \return the current id-limit / bound
    virtual uint32_t GetBound() = 0;

    /// Allocate a shader export
    /// \tparam T the allocation type, must be of a schema shader-export type
    /// \return the allocation identifier
    template<typename T>
    ShaderExportID Allocate() {
        return Allocate(ShaderExportTypeInfo::FromType<T>());
    }
};
