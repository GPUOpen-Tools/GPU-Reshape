#pragma once

/// Placement of an allocation
enum class AllocationResidency {
    /// Resident on GPU memory
    Device,

    /// Resident on host (CPU) memory
    Host
};
