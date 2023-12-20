#pragma once

enum class ApplicationLaunchType {
    None,

    /// Application is a standard executable
    /// Expecting a file path as identifier
    Executable,

    /// Application is a steam-app
    /// Expecting a steam-id as identifier
    Steam
};
