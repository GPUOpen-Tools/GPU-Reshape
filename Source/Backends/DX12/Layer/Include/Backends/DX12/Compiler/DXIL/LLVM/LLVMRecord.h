#pragma once

// Std
#include <cstdint>

struct LLVMRecord {
    /// Identifier of this record, may be reserved
    uint32_t id{~0u};

    /// Number of operands within this record
    uint32_t opCount{0};

    /// All operands
    uint64_t* ops{nullptr};

    /// Optional blob size associated
    uint64_t blobSize{0};

    /// Blob size, lifetime owned by parent module
    const uint8_t* blob{nullptr};
};
