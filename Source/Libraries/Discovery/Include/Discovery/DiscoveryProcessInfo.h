#pragma once

struct DiscoveryProcessInfo {
    /// Path of the application
    const char* applicationPath{nullptr};

    /// Working directory of the application
    const char* workingDirectoryPath{nullptr};

    /// All command line arguments given to the application
    const char* arguments{nullptr};
};
