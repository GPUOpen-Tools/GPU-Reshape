#pragma once

// Std
#include <cstdint>

enum class LLVMRecordAbbreviationType {
    /// Not an abbreviated record
    None,

    /// Abbreviation defined within the block metadata
    BlockMetadata,

    /// Abbreviation defined within the block
    BlockLocal,
};

struct LLVMRecordAbbreviation {
    /// Type, may be None
    LLVMRecordAbbreviationType type{LLVMRecordAbbreviationType::None};

    /// Index within the type
    uint32_t abbreviationId{~0u};
};
