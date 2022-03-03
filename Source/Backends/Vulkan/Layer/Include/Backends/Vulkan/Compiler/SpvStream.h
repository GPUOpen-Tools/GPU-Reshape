#pragma once

// Layer
#include "SpvInstruction.h"

// Backend
#include <Backend/IL/Source.h>

// Common
#include <Common/Assert.h>

// Std
#include <vector>

/// Simple appendable spirv stream
struct SpvStream {
    /// Constructor
    /// \param code the source data
    SpvStream(const uint32_t* code = nullptr) : code(code) {

    }

    /// Append a chunk of data
    /// \param ptr the data
    /// \param wordCount the word count of [ptr]
    void AppendData(const void* ptr, uint32_t wordCount) {
        auto* wordPtr = static_cast<const uint32_t*>(ptr);
        stream.insert(stream.end(), wordPtr, wordPtr + wordCount);
    }

    /// Append a value
    /// \param value the value to be appended
    template<typename T>
    void Append(const T& value) {
        auto* words = reinterpret_cast<const uint32_t*>(&value);
        stream.insert(stream.end(), words, words + sizeof(T) / sizeof(uint32_t));
    }

    /// Allocate an appended type
    /// \tparam T the type to be appended
    /// \return the allocated type, invalidated upon next insertion
    SpvInstruction& Allocate(SpvOp op, uint32_t wordCount) {
        size_t offset = stream.size();
        stream.resize(stream.size() + wordCount);
        return *new (&stream[offset]) SpvInstruction(op, wordCount);
    }

    /// Template a source instruction
    /// \tparam T the templated instruction type
    /// \param source the source word offset
    /// \return the allocated instruction
    SpvInstruction& Template(const IL::Source& source) {
        ASSERT(source.IsValid(), "Cannot template instruction without source");
        auto* instruction = reinterpret_cast<const SpvInstruction*>(code + source.codeOffset);

        size_t offset = stream.size();
        stream.insert(stream.end(), code + source.codeOffset, code + source.codeOffset + instruction->GetWordCount());
        return *reinterpret_cast<SpvInstruction*>(&stream[offset]);
    }

    /// Template a source instruction if the source operand is valid, otherwise allocate a new one
    /// \tparam T the templated instruction type
    /// \param source the source word offset
    /// \return the allocated instruction
    SpvInstruction& TemplateOrAllocate(SpvOp op, uint32_t wordCount, const IL::Source& source) {
        if (!source.IsValid()) {
            return Allocate(op, wordCount);
        }

        return Template(source);
    }

    /// Preallocate the stream
    /// \param wordCount preallocated word count
    void Reserve(uint64_t wordCount) {
        stream.reserve(wordCount);
    }

    /// Clear this stream
    void Clear() {
        stream.clear();
    }

    /// Get the word data
    const uint32_t* GetData() const {
        return stream.data();
    }

    /// Get the word count
    size_t GetWordCount() const {
        return stream.size();
    }

private:
    /// Source data
    const uint32_t* code{nullptr};

    /// Stream data
    std::vector<uint32_t> stream;
};
