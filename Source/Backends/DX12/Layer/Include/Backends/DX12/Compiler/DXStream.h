#pragma once

// Backend
#include <Backend/IL/Source.h>

// Common
#include <Common/Assert.h>

// Std
#include <vector>

/// Simple appendable DX stream
struct DXStream {
    /// Constructor
    /// \param code the source data
    DXStream(const uint32_t* code = nullptr) : code(code) {

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
