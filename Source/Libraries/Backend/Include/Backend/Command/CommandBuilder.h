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
    void SetDescriptorData(ShaderDataID id, uint32_t value) {
        buffer.Add(SetDescriptorDataCommand {
            .id = id,
            .value = value
        });
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

    /// Dispatch the bound shader program
    /// \param groupCountX number of groups X
    /// \param groupCountY number of groups Y
    /// \param groupCountZ number of groups Z
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        buffer.Add(DispatchCommand {
            .groupCountX = groupCountX,
            .groupCountY = groupCountY,
            .groupCountZ = groupCountZ
        });
    }

private:
    CommandBuffer& buffer;
};
