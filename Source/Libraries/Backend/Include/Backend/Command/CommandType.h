#pragma once

enum class CommandType {
    /// Shader program
    SetShaderProgram,

    /// Data
    SetEventData,
    SetDescriptorData,

    /// Invokes
    Dispatch
};
