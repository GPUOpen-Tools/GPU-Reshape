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
#include <Common/Containers/LinearBlockAllocator.h>

// Layer
#include <Backends/DX12/DeepCopy.Gen.h>

struct PipelineSubObjectWriter {
    static constexpr uint32_t kAlign = sizeof(void*);

    /// Constructor
    /// \param allocators underlying allocators
    PipelineSubObjectWriter(const Allocators& allocators) : stream(allocators), allocator(allocators) {
        
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

    /// Append a chunk with sub-data re-serialization
    /// \param type type of the chunk
    /// \param blob blob to be written
    /// \param length byte length of the blob
    void AppendChunk(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, const void* blob, size_t length) {
        size_t offset = stream.size();

        // Append chunk header
        Append(blob, length);

        // Chunk offset
        void* dest = &stream[offset];

        // Get sub-data length
        size_t subDataLength = SerializeOpaque(type, blob, dest, nullptr);
        if (!subDataLength) {
            return;
        }

        // Serialize sub-data into separate allocation
        uint8_t* subData = allocator.AllocateArray<uint8_t>(static_cast<uint32_t>(subDataLength));
        ENSURE(SerializeOpaque(type, blob, dest, subData) == subDataLength, "Mismatched sub-data serialization");
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
    void Swap(PipelineSubObjectWriter& out) {
        stream.swap(out.stream);
        allocator.Swap(out.allocator);
    }

    /// Get the stream description
    D3D12_PIPELINE_STATE_STREAM_DESC GetDesc() {
        D3D12_PIPELINE_STATE_STREAM_DESC desc{};
        desc.SizeInBytes = stream.size();
        desc.pPipelineStateSubobjectStream = stream.data();
        return desc;
    }

private:
    /// Serialize an opque type
    /// \param type chunk type
    /// \param source source data, used for serialization
    /// \param dest serialization target
    /// \param blob optional, sub-data blob
    /// \return byte size of sub-data
    size_t SerializeOpaque(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, const void* source, void* dest, void* blob) {
        switch (type) {
            default:
                return 0;
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
                return Serialize(*static_cast<const D3D12_STREAM_OUTPUT_DESC*>(source), *static_cast<D3D12_STREAM_OUTPUT_DESC*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
                return Serialize(*static_cast<const D3D12_BLEND_DESC*>(source), *static_cast<D3D12_BLEND_DESC*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
                return Serialize(*static_cast<const D3D12_RASTERIZER_DESC*>(source), *static_cast<D3D12_RASTERIZER_DESC*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
                return Serialize(*static_cast<const D3D12_DEPTH_STENCIL_DESC*>(source), *static_cast<D3D12_DEPTH_STENCIL_DESC*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
                return Serialize(*static_cast<const D3D12_INPUT_LAYOUT_DESC*>(source), *static_cast<D3D12_INPUT_LAYOUT_DESC*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                return Serialize(*static_cast<const D3D12_RT_FORMAT_ARRAY*>(source), *static_cast<D3D12_RT_FORMAT_ARRAY*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                return Serialize(*static_cast<const D3D12_DEPTH_STENCIL_DESC1*>(source), *static_cast<D3D12_DEPTH_STENCIL_DESC1*>(dest), blob);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                return Serialize(*static_cast<const D3D12_VIEW_INSTANCING_DESC*>(source), *static_cast<D3D12_VIEW_INSTANCING_DESC*>(dest), blob);
        }
    }

private:
    /// Underlying description
    Vector<uint8_t> stream;

    /// Internal allocator
    LinearBlockAllocator<4096> allocator;
};
