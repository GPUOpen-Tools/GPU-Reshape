#pragma once

// Std
#include <atomic>

struct ShaderCompilerDiagnostic {
    /// Constructor
    ShaderCompilerDiagnostic() = default;

    /// Copy constructor
    ShaderCompilerDiagnostic(const ShaderCompilerDiagnostic& other) : failedJobs(other.failedJobs.load()) {
        
    }

    /// Total number of failed job
    std::atomic<uint64_t> failedJobs{0};
};
