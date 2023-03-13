#pragma once

// Layer
#include "SpvRecord.h"
#include "SpvStream.h"

// Std
#include <vector>

struct SpvBlock {
    /// Write records to a stream
    /// \param out destination stream
    void Write(SpvStream& out) {
        // Preallocate
        uint32_t* words = out.AllocateRaw(GetWordCount());

        // Write all records
        for (const SpvRecord& record : records) {
            if (record.IsDeprecated()) {
                continue;
            }
            
            words[0] = record.lowWordCountHighOpCode;
            std::memcpy(&words[1], record.operands, sizeof(uint32_t) * record.GetOperandCount());
            words += record.GetWordCount();
        }
    }

    /// Get the expected word count
    /// \return word count
    uint32_t GetWordCount() {
        uint32_t count = 0;

        // Summarize
        for (const SpvRecord& record : records) {
            if (record.IsDeprecated()) {
                continue;
            }
            
            count += record.GetWordCount();
        }

        return count;
    }

    /// All records
    std::vector<SpvRecord> records;
};
