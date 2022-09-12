#pragma once

// Layer
#include "LLVMBlock.h"

struct LLVMRecordView {
    LLVMRecordView(LLVMBlock* block, uint32_t offset) : block(block), offset(offset) {

    }

    /// Accessor
    LLVMRecord* operator->() const {
        return &block->records[offset];
    }

    /// Originating block
    LLVMBlock* block;

    /// Source offset
    uint32_t offset;
};
