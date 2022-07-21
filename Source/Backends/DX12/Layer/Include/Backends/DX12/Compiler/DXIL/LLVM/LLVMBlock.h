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
    /// Check if this block is of reserved id
    template<typename T>
    bool Is(T value) const {
        return value == static_cast<T>(id);
    }

    /// Interpret the id as a reserved type
    template<typename T>
    T As() const {
        return static_cast<T>(id);
    }

    /// Identifier of this block, may be reserved
    uint32_t id{~0u};

    /// Abbreviation size
    uint32_t abbreviationSize{~0u};

    /// First scan block length
    uint32_t blockLength{~0u};

    /// All child blocks
    TrivialStackVector<LLVMBlock*, 32> blocks;

    /// All records within this block
    TrivialStackVector<LLVMRecord, 32> records;

    /// All abbreviations local to this block
    TrivialStackVector<LLVMAbbreviation, 32> abbreviations;

    /// Optional metadata
    LLVMBlockMetadata* metadata{nullptr};
};
