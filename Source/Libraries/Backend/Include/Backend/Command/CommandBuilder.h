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
