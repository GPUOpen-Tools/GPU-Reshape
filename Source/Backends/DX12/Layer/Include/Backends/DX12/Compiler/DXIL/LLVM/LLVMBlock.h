#pragma once

// Layer
#include "LLVMRecord.h"
#include "LLVMAbbreviation.h"

// Std
#include <cstdint>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Forward declarations
struct LLVMBlockMetadata;

struct LLVMBlock {
    /// Identifier of this block, may be reserved
    uint32_t id{~0u};

    /// All child blocks
    TrivialStackVector<LLVMBlock*, 32> blocks;

    /// All records within this block
    TrivialStackVector<LLVMRecord, 32> records;

    /// All abbreviations local to this block
    TrivialStackVector<LLVMAbbreviation, 32> abbreviations;

    /// Optional metadata
    LLVMBlockMetadata* metadata{nullptr};
};
