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

#include "CommandBuffer.h"
#include "CommandLimits.h"

struct CommandBuilder {
    CommandBuilder(CommandBuffer& buffer) : buffer(buffer) {

    }

    /// Set the shader program
    /// \param id program to be set
    void SetShaderProgram(ShaderProgramID id) {
        buffer.Add(SetShaderProgramCommand {
            .id = id
        });
    }

    /// Set event data
    /// \param id event data to be mapped
    /// \param value value to be written
    void SetEventData(ShaderDataID id, uint32_t value) {
        buffer.Add(SetEventDataCommand {
            .id = id,
            .value = value
        });
    }

    /// Set descriptor data
    /// \param id event data to be mapped
    /// \param value value to be written
    template<typename T>
    void SetDescriptorData(ShaderDataID id, const T& value) {
        ASSERT(sizeof(T) < (1u << 16u) - sizeof(SetDescriptorDataCommand), "Inline staging buffer exceeds max size");

        SetDescriptorDataCommand command;
        command.commandSize += sizeof(value);
        command.id = id;
        
        buffer.Append(&command, sizeof(command));
        buffer.Append(&value, sizeof(value));
        buffer.Increment();
    }

    /// Stage a buffer
    /// \param id buffer id
    /// \param offset offset into buffer
    /// \param length length of data to stage
    /// \param data data to stage
    /// \param flags optional flags
    void StageBuffer(ShaderDataID id, size_t offset, size_t length, const void* data, StageBufferFlagSet flags = StageBufferFlag::None) {
        ASSERT(length < (1u << 16u) - sizeof(StageBufferCommand), "Inline staging buffer exceeds max size");

        StageBufferCommand command;
        command.commandSize += static_cast<uint32_t>(length);
        command.id = id;
        command.offset = offset;
        command.flags = flags;

        buffer.Append(&command, sizeof(command));
        buffer.Append(data, length);
        buffer.Increment();
    }

    /// Clear a buffer
    /// \param id buffer id
    /// \param offset offset into buffer
    /// \param length length of data to stage
    /// \param value dword to be written
    void ClearBuffer(ShaderDataID id, size_t offset, size_t length, uint32_t value) {
        buffer.Add(ClearBufferCommand {
            .id = id,
            .offset = offset,
            .length = length,
            .value = value
        });
    }

    /// Dispatch the bound shader program
    /// \param groupCountX number of groups X
    /// \param groupCountY number of groups Y
    /// \param groupCountZ number of groups Z
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        ASSERT(groupCountX <= kMaxDispatchThreadGroupPerDimension && groupCountY <= kMaxDispatchThreadGroupPerDimension && groupCountZ <= kMaxDispatchThreadGroupPerDimension,
               "Exceeded maximum thread groups per dimension");
        
        buffer.Add(DispatchCommand {
            .groupCountX = groupCountX,
            .groupCountY = groupCountY,
            .groupCountZ = groupCountZ
        });
    }

    /// Discard a resource
    /// \param puid the resource puid
    void Discard(uint32_t puid) {
        buffer.Add(DiscardCommand {
            .puid = puid
        });
    }

    /// Full pipeline UAV barrier
    void UAVBarrier() {
        buffer.Add(UAVBarrierCommand{});
    }

private:
    CommandBuffer& buffer;
};
