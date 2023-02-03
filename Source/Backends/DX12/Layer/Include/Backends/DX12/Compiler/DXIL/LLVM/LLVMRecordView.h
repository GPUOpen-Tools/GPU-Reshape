#pragma once

// Layer
#include "LLVMBlock.h"

struct LLVMRecordView {
    LLVMRecordView(LLVMBlock* block = nullptr, uint32_t offset = 0) : block(block), offset(offset) {

    }

    /// Accessor
    LLVMRecord* operator->() const {
        return &block->records[offset];
    }

    /// Check for validity
    operator bool() const {
        return block != nullptr;
    }

    /// Originating block
    LLVMBlock* block;

    /// Source offset
    uint32_t offset;
};
