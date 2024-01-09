// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include "DX12.h"
#include "States/PipelineType.h"

struct PipelineSubObjectReader {
    static constexpr uint32_t kAlign = sizeof(void*);

    /// Constructor
    /// \param desc given description
    PipelineSubObjectReader(const D3D12_PIPELINE_STATE_STREAM_DESC* desc) : desc(desc) {

    }

    /// Get the size of a type
    /// \param type given type
    /// \return byte size, 0 if invalid
    static uint64_t GetSize(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type) {
        switch (type) {
            default:
                ASSERT(false, "Invalid sub-object type");
                return 0;
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
                return sizeof(ID3D12RootSignature*);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
                return sizeof(D3D12_SHADER_BYTECODE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
                return sizeof(D3D12_STREAM_OUTPUT_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
                return sizeof(D3D12_BLEND_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
                return sizeof(uint32_t);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
                return sizeof(D3D12_RASTERIZER_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
                return sizeof(D3D12_DEPTH_STENCIL_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
                return sizeof(D3D12_INPUT_LAYOUT_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
                return sizeof(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
                return sizeof(D3D12_PRIMITIVE_TOPOLOGY_TYPE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                return sizeof(D3D12_RT_FORMAT_ARRAY);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
                return sizeof(DXGI_FORMAT);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
                return sizeof(DXGI_SAMPLE_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
                return sizeof(uint32_t);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO:
                return sizeof(D3D12_CACHED_PIPELINE_STATE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
                return sizeof(D3D12_PIPELINE_STATE_FLAGS);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                return sizeof(D3D12_DEPTH_STENCIL_DESC1);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                return sizeof(D3D12_VIEW_INSTANCING_DESC);
        }
    }

    /// Get the alignment of a type
    /// \param type given type
    /// \return alignment, 0 if invalid
    static uint64_t GetAlignOf(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type) {
        switch (type) {
            default:
                ASSERT(false, "Invalid sub-object type");
                return 0;
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE:
                return alignof(ID3D12RootSignature*);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
                return alignof(D3D12_SHADER_BYTECODE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
                return alignof(D3D12_STREAM_OUTPUT_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
                return alignof(D3D12_BLEND_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
                return alignof(uint32_t);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
                return alignof(D3D12_RASTERIZER_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
                return alignof(D3D12_DEPTH_STENCIL_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
                return alignof(D3D12_INPUT_LAYOUT_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
                return alignof(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
                return alignof(D3D12_PRIMITIVE_TOPOLOGY_TYPE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                return alignof(D3D12_RT_FORMAT_ARRAY);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
                return alignof(DXGI_FORMAT);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
                return alignof(DXGI_SAMPLE_DESC);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
                return alignof(uint32_t);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO:
                return alignof(D3D12_CACHED_PIPELINE_STATE);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
                return alignof(D3D12_PIPELINE_STATE_FLAGS);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                return alignof(D3D12_DEPTH_STENCIL_DESC1);
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                return alignof(D3D12_VIEW_INSTANCING_DESC);
        }
    }

    /// Should a type be aligned?
    /// \param type given type
    /// \return requires alignment?
    static bool ShouldAlign(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type) {
        return GetAlignOf(type) != sizeof(uint32_t);
    }

    /// Get the type of a pipeline
    /// \return type
    PipelineType GetPipelineType() {
        uint8_t* byteStream = static_cast<uint8_t*>(desc->pPipelineStateSubobjectStream);

        // Read all objects
        for (uint64_t offset = 0; offset < desc->SizeInBytes;) {
            auto type = *reinterpret_cast<const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(byteStream + offset);

            // Stage type?
            switch (type) {
                default:
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
                    return PipelineType::Compute;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
                    return PipelineType::Graphics;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
                    ASSERT(false, "Mesh shaders not supported");
                    return PipelineType::None;
            }

            // Next!
            offset += sizeof(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE) + PipelineSubObjectReader::GetSize(type);

            // Align to void*
            if (offset % kAlign) {
                offset += kAlign - offset % kAlign;
            }
        }

        // Invalid stream!
        ASSERT(false, "Invalid stream, failed to deduce type of pipeline");
        return PipelineType::None;
    }

    /// Consume a given value
    /// \tparam T type to consume, used for byte width
    /// \return type ref
    template<typename T>
    T& Consume() {
        ASSERT(consumeOffset + sizeof(T) <= desc->SizeInBytes, "Sub-object consume out of bounds");
        T& value = *reinterpret_cast<T*>(static_cast<uint8_t*>(desc->pPipelineStateSubobjectStream) + consumeOffset);
        consumeOffset += sizeof(T);
        return value;
    }

    /// Consume an aligned (void*) value
    /// \tparam T type to consume, used for byte width
    /// \return type ref
    template<typename T>
    T& AlignedConsume() {
        Align();
        return Consume<T>();
    }

    /// Consume a given value
    /// \tparam T type to consume, used for byte width
    /// \param outOffset previous offset
    /// \return type ref
    template<typename T>
    T& ConsumeWithOffset(uint64_t& outOffset) {
        outOffset = consumeOffset;
        return Consume<T>();
    }

    /// Consume an aligned (void*) value
    /// \tparam T type to consume, used for byte width
    /// \param outOffset previous offset
    /// \return type ref
    template<typename T>
    T& AlignedConsumeWithOffset(uint64_t& outOffset) {
        Align();
        return ConsumeWithOffset<T>(outOffset);
    }

    /// Align the offset to void*
    void Align() {
        if (consumeOffset % kAlign) {
            consumeOffset += kAlign - consumeOffset % kAlign;
        }
    }

    /// Skip a number of bytes
    /// \param size byte count
    const void* Skip(uint64_t size) {
        const void* data = static_cast<uint8_t*>(desc->pPipelineStateSubobjectStream) + consumeOffset;
        
        // Align offset
        ASSERT(consumeOffset + size <= desc->SizeInBytes, "Sub-object consume out of bounds");
        consumeOffset += size;

        // OK
        return data;
    }

    /// Is the stream in a good state? (i.e. not EOS)
    bool IsGood() const {
        return consumeOffset < desc->SizeInBytes;
    }

private:
    /// Underlying description
    const D3D12_PIPELINE_STATE_STREAM_DESC* desc{nullptr};

    /// Current consume offset
    uint64_t consumeOffset{0};
};
