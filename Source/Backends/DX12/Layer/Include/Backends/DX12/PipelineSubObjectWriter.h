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

// Common
#include <Common/Allocator/Vector.h>

struct PipelineSubObjectWriter {
    static constexpr uint32_t kAlign = sizeof(void*);

    /// Constructor
    /// \param allocators underlying allocators
    PipelineSubObjectWriter(const Allocators& allocators) : stream(allocators) {
        
    }

    /// Reserve the container memory
    /// \param size size to be reserved
    void Reserve(size_t size) {
        stream.reserve(size);
    }

    /// Write a given value
    /// \param value value to be written, size assumed from top type
    template<typename T>
    void Write(const T& value) {
        auto data = reinterpret_cast<const uint8_t*>(&value);
        stream.insert(stream.end(), data, data + sizeof(value));
    }

    /// Append a blob
    /// \param blob blob to be written
    /// \param length byte length of the blob
    void Append(const void* blob, size_t length) {
        auto data = static_cast<const uint8_t*>(blob);
        stream.insert(stream.end(), data, data + length);
    }

    /// Write a given value with internal alignment
    /// \param value value to be written, size assumed from top type
    template<typename T>
    void WriteAligned(const T& value) {
        Align();
        Write(value);
    }

    /// Append a blob with internal alignment
    /// \param blob blob to be written
    /// \param length byte length of the blob
    void AppendAligned(const void* blob, size_t length) {
        Align();
        Append(blob, length);
    }

    /// Align the offset to void*
    void Align() {
        size_t unalignment = stream.size() % kAlign;

        // Any unaligned?
        if (unalignment) {
            stream.resize(stream.size() + (kAlign - unalignment));
        }
    }

    /// Swap this container
    /// \param out destination container
    void Swap(Vector<uint8_t>& out) {
        stream.swap(out);
    }

    /// Get the stream description
    D3D12_PIPELINE_STATE_STREAM_DESC GetDesc() {
        D3D12_PIPELINE_STATE_STREAM_DESC desc{};
        desc.SizeInBytes = stream.size();
        desc.pPipelineStateSubobjectStream = stream.data();
        return desc;
    }

private:
    /// Underlying description
    Vector<uint8_t> stream;
};
