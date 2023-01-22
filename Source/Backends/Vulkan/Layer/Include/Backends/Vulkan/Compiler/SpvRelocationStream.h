#pragma once

// Layer
#include "SpvStream.h"

// Std
#include <cstdint>
#include <list>
#include <vector>

/// Single relocation block, represents a region of source word region that is to be replaced
struct SpvRelocationBlock {
    /// Constructor
    /// \param code the source data
    /// \param span the source byte range
    SpvRelocationBlock(const uint32_t *code) : stream(code) {

    }

    /// Constructor
    /// \param code the source data
    /// \param span the source byte range
    SpvRelocationBlock(const uint32_t *code, const IL::SourceSpan &span) : span(span), stream(code) {

    }

    /// Constructor
    /// \param code the source data
    /// \param span the source byte range
    /// \param ptr the constant data to be written, size must be source byte range
    SpvRelocationBlock(const uint32_t *code, const IL::SourceSpan &span, const void* ptr) : span(span), ptr(ptr), stream(code) {

    }

    /// Source byte range
    IL::SourceSpan span;

    /// Optional, the constant data
    const void* ptr{nullptr};

    /// The spirv data stream, ignored if [ptr] is valid
    SpvStream stream;
};

/// Relocation stream, for fast spirv block replacement
struct SpvRelocationStream {
    /// Constructor
    /// \param code the source data
    /// \param wordCount the source word count
    SpvRelocationStream(const uint32_t *code, uint32_t wordCount) : code(code), wordCount(wordCount) {

    }

    /// Allocate a new block
    /// \param span the source byte span
    /// \return the relocation block
    SpvRelocationBlock &AllocateBlock() {
        return blocks.emplace_back(code);
    }

    /// Allocate a new block
    /// \param span the source byte span
    /// \return the relocation block
    SpvRelocationBlock &AllocateBlock(const IL::SourceSpan &span) {
        return blocks.emplace_back(code, span);
    }

    /// Allocate a new fixed block
    /// \param span the source byte span
    /// \param ptr the constant data to be written during stitching
    /// \return the relocation block
    SpvRelocationBlock &AllocateFixedBlock(const IL::SourceSpan &span, const void* ptr) {
        return blocks.emplace_back(code, span, ptr);
    }

    /// Stitch this stream
    /// \param out the output vector
    void Stitch(std::vector<uint32_t> &out) {
        uint32_t offset = 0;

        // Stitch all blocks
        for (auto it = blocks.begin(); it != blocks.end(); it++) {
            // Pre-stitching!
            if (offset != it->span.begin) {
                // ... stitched data
                // -- Last known offset  ----- \
                //                             |
                //                             | Missing data for stitch block
                //                             |
                // -- Start of stitch block -- /
                // ... block data
                out.insert(out.end(), code + offset, code + it->span.begin);
            }

            // Insert block data
            if (it->ptr) {
                auto* wordPtr = static_cast<const uint32_t*>(it->ptr);
                out.insert(out.end(), wordPtr, wordPtr + (it->span.end - it->span.begin));
            } else {
                out.insert(out.end(), it->stream.GetData(), it->stream.GetData() + it->stream.GetWordCount());
            }

            // Set offset
            offset = it->span.end;
        }

        // End stitching
        if (!blocks.empty() && blocks.back().span.end != wordCount) {
            uint32_t blockEnd = blocks.back().span.end;

            // ... block data
            // -- Block end  ------ \
            //                      |
            //                      | Missing data for end of program
            //                      |
            // -- End of program -- /
            out.insert(out.end(), code + blockEnd, code + wordCount);
        }
    }

private:
    /// Source data
    const uint32_t *code;

    /// Source word count
    uint32_t wordCount;

    /// All relocation blocks
    std::list<SpvRelocationBlock> blocks;
};
