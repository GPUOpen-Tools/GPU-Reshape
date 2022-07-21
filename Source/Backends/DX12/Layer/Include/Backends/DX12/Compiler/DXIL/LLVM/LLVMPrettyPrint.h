#pragma once

// Std
#include <ostream>

// Layer
#include "LLVMBlock.h"
#include "LLVMRecord.h"
#include "LLVMAbbreviation.h"

/// Standard printer
struct LLVMPrettyPrintContext {
    LLVMPrettyPrintContext(std::ostream& out, uint32_t indent = 0) : out(out), indent(indent) {

    }

    /// Add a new line
    std::ostream& Line() {
        for (uint32_t i = 0; i < indent; i++) {
            out << "\t";
        }

        return out;
    }

    /// Tab this stream
    LLVMPrettyPrintContext Tab() {
        return LLVMPrettyPrintContext(out, indent + 1);
    }

    /// Destination stream
    std::ostream& out;

    /// Current indentation level
    uint32_t indent;
};

/// Pretty printers
void PrettyPrint(LLVMPrettyPrintContext out, const LLVMRecord& record);
void PrettyPrint(LLVMPrettyPrintContext out, const LLVMAbbreviation& abbreviation);
void PrettyPrint(LLVMPrettyPrintContext out, const LLVMBlock* block);
