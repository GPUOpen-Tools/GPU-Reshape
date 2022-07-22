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
    DXStream(const uint8_t* code = nullptr) : code(code) {

    }

    /// Append a chunk of data
    /// \param ptr the data
    /// \param wordCount the word count of [ptr]
    void AppendData(const void* ptr, uint32_t byteCount) {
        auto* chPtr = static_cast<const uint8_t*>(ptr);
        stream.insert(stream.end(), chPtr, chPtr + byteCount);
    }

    /// Append a value
    /// \param value the value to be appended
    template<typename T>
    void Append(const T& value) {
        auto* chPtr = reinterpret_cast<const uint8_t*>(&value);
        stream.insert(stream.end(), chPtr, chPtr + sizeof(T));
    }

    /// Pop the next 64 bit word
    /// \param value initial value
    /// \return 64 word address
    uint64_t* NextWord64(uint64_t value = 0) {
        size_t offset = stream.size();
        stream.resize(stream.size() + sizeof(uint64_t));

        // Initialize
        auto* word64 = reinterpret_cast<uint64_t*>(&stream[offset]);
        *word64 = value;
        return word64;
    }

    /// Reserve up to a byte count
    void Reserve(uint32_t byteCount) {
        stream.reserve(byteCount);
    }

    /// Resize the container to a byte count
    void Resize(uint32_t byteCount) {
        stream.resize(byteCount);
    }

    /// Clear this stream
    void Clear() {
        stream.clear();
    }

    /// Get the word data
    template<typename T = uint8_t>
    T* GetMutableData() {
        return reinterpret_cast<T*>(stream.data());
    }

    /// Get the word data
    template<typename T = uint8_t>
    const T* GetData() const {
        return reinterpret_cast<const T*>(stream.data());
    }

    /// Get the byte size of this stream
    size_t GetByteSize() const {
        return stream.size() * sizeof(uint8_t);
    }

private:
    /// Source data
    const uint8_t* code{nullptr};

    /// Stream data
    std::vector<uint8_t> stream;
};
