#pragma once

// Std
#include <atomic>

struct PipelineCompilerDiagnostic {
    /// Constructor
    PipelineCompilerDiagnostic() = default;

    /// Copy constructor
    PipelineCompilerDiagnostic(const PipelineCompilerDiagnostic& other) : failedJobs(other.failedJobs.load()) {
        
    }

    /// Total number of failed jobs
    std::atomic<uint64_t> failedJobs{0};
};
