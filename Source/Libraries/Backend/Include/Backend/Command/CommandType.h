#pragma once

enum class CommandType {
    /// Shader program
    SetShaderProgram,

    /// Immediate data
    SetEventData,
    SetDescriptorData,

    /// Resources
    StageBuffer,

    /// Invokes
    Dispatch
};
