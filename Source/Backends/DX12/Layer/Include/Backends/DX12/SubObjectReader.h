#pragma once

// Layer
#include "DX12.h"
#include "States/PipelineType.h"

struct SubObjectReader {
    static constexpr uint32_t kAlign = sizeof(void*);

    /// Constructor
    /// \param desc given description
    SubObjectReader(const D3D12_PIPELINE_STATE_STREAM_DESC* desc) : desc(desc) {

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
            offset += sizeof(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE) + SubObjectReader::GetSize(type);

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
    T& ConsumeWithOffset(uint64_t outOffset) {
        outOffset = consumeOffset;
        return Consume<T>();
    }

    /// Consume an aligned (void*) value
    /// \tparam T type to consume, used for byte width
    /// \param outOffset previous offset
    /// \return type ref
    template<typename T>
    T& AlignedConsumeWithOffset(uint64_t outOffset) {
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
    void Skip(uint64_t size) {
        ASSERT(consumeOffset + size <= desc->SizeInBytes, "Sub-object consume out of bounds");
        consumeOffset += size;
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
