#pragma once

enum class CommandType {
    /// Shader program
    SetShaderProgram,

    /// Data
    SetEventData,

    /// Invokes
    Dispatch
};
