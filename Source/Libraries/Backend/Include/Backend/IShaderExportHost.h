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

    /// Allocate a shader export
    /// \tparam T the allocation type, must be of a schema shader-export type
    /// \return the allocation identifier
    template<typename T>
    ShaderExportID Allocate() {
        return Allocate(ShaderExportTypeInfo::FromType<T>());
    }
};
