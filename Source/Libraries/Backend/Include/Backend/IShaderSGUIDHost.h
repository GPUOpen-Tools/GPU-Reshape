#pragma once

#include "ShaderExport.h"
#include "ShaderSourceMapping.h"

// Common
#include <Common/IComponent.h>

// Std
#include <string_view>

// Forward declarations
namespace IL {
    struct Program;
    struct ConstOpaqueInstructionRef;
}

class IShaderSGUIDHost : public TComponent<IShaderSGUIDHost> {
public:
    COMPONENT(IShaderSGUIDHost);

    /// Bind an instruction to the sguid host
    /// \param program the program of [instruction]
    /// \param instruction the instruction to be bound
    /// \return the shader guid value
    virtual ShaderSGUID Bind(const IL::Program& program, const IL::ConstOpaqueInstructionRef& instruction) = 0;

    /// Get the mapping for a given sguid
    /// \param sguid
    /// \return
    virtual ShaderSourceMapping GetMapping(ShaderSGUID sguid) = 0;

    /// Get the source code for a binding
    /// \param sguid the shader sguid
    /// \return the source code, empty if not found
    virtual std::string_view GetSource(ShaderSGUID sguid) = 0;

    /// Get the source code for a mapping
    /// \param mapping a mapping allocated from ::Bind
    /// \return the source code, empty if not found
    virtual std::string_view GetSource(const ShaderSourceMapping& mapping) = 0;
};
