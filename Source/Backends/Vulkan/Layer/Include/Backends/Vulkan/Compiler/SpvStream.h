#pragma once

// Layer
#include "SpvInstruction.h"

// Backend
#include <Backend/IL/Source.h>

// Std
#include <vector>

/// Simple appendable spirv stream
struct SpvStream {
    /// Constructor
    /// \param code the source data
    SpvStream(const uint32_t* code) : code(code) {

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
    template<typename T>
    T* Allocate() {
        uint32_t wordCount = sizeof(T) / sizeof(uint32_t);

        size_t offset = stream.size();
        stream.resize(stream.size() + wordCount);

        return new (reinterpret_cast<T*>(&stream[offset])) T();
    }

    /// Template a source instruction
    /// \tparam T the templated instruction type
    /// \param source the source word offset
    /// \return the allocated instruction
    template<typename T = SpvInstruction>
    T* Template(uint32_t source) {
        ASSERT(source != IL::InvalidSource, "Cannot template instruction without source");
        auto* instruction = reinterpret_cast<const SpvInstruction*>(code + source);

        size_t offset = stream.size();
        stream.insert(stream.end(), code + source, code + source + instruction->GetWordCount());
        return reinterpret_cast<T*>(&stream[offset]);
    }

    /// Template a source instruction if the source operand is valid, otherwise allocate a new one
    /// \tparam T the templated instruction type
    /// \param source the source word offset
    /// \return the allocated instruction
    template<typename T = SpvInstruction>
    T* TemplateOrAllocate(uint32_t source) {
        if (source == IL::InvalidSource) {
            return Allocate<T>();
        }

        return Template<T>(source);
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
