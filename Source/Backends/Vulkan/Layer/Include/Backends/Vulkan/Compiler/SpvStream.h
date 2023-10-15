// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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

    /// Allocate a block of words
    /// \param count word count
    /// \return base address
    uint32_t* AllocateRaw(uint32_t count) {
        uint32_t offset = static_cast<uint32_t>(stream.size());
        stream.resize(stream.size() + count);
        return stream.data() + offset;
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

    /// Get an instruction
    /// \param source given source
    /// \return instruction
    const SpvInstruction* GetInstruction(const IL::Source& source) {
        ASSERT(source.IsValid(), "Cannot template instruction without source");
        return reinterpret_cast<const SpvInstruction*>(code + source.codeOffset);
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

    /// Get an instruction at an offset
    SpvInstruction& Get(uint32_t offset) {
        return *reinterpret_cast<SpvInstruction*>(stream.data() + offset);
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
