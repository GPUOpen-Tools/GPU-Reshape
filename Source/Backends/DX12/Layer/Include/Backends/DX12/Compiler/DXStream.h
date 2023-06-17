// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include "Tags.h"

// Backend
#include <Backend/IL/Source.h>

// Common
#include <Common/Allocator/Vector.h>
#include <Common/Assert.h>

// Std
#include <vector>

/// Simple appendable DX stream
struct DXStream {
    /// Constructor
    /// \param code the source data
    DXStream(const Allocators& allocators, const uint8_t* code = nullptr) : code(code), stream(allocators.Tag(kAllocModuleDXStream)) {

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
    size_t Append(const T& value) {
        uint64_t offset = GetOffset();

        auto* chPtr = reinterpret_cast<const uint8_t*>(&value);
        stream.insert(stream.end(), chPtr, chPtr + sizeof(T));

        return offset;
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

    /// Write a value into the stream, entire range must be available
    /// \param offset byte offset
    /// \param value value to be written, must be available
    template<typename T>
    void Write(size_t offset, const T& value) {
        std::memcpy(stream.data() + offset, &value, sizeof(T));
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

    /// Get the word data
    template<typename T = uint8_t>
    T* GetMutableDataAt(size_t offset) {
        return reinterpret_cast<T*>(stream.data() + offset);
    }

    /// Get the word data
    template<typename T = uint8_t>
    const T* GetDataAt(size_t offset) const {
        return reinterpret_cast<const T*>(stream.data() + offset);
    }

    /// Get the byte size of this stream
    uint32_t GetByteSize() const {
        return static_cast<uint32_t>(stream.size() * sizeof(uint8_t));
    }

    /// Get the offset of this stream
    uint32_t GetOffset() const {
        return GetByteSize();
    }

private:
    /// Source data
    const uint8_t* code{nullptr};

    /// Stream data
    Vector<uint8_t> stream;
};
