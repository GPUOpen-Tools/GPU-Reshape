#pragma once

// Backend
#include <Backend/Command/CommandType.h>
#include <Backend/ShaderProgram/ShaderProgram.h>
#include <Backend/ShaderData/ShaderData.h>

// Common
#include <Common/Assert.h>
#include <Common/Enum.h>

// Std
#include <cstdint>

struct Command {
    /// Type of the command
    uint32_t commandType : 16;

    /// Size of the command
    uint32_t commandSize : 16;

    /// Constructor
    /// \param type type given
    /// \param size size given
    Command(CommandType type, uint32_t size) : commandType(static_cast<uint32_t>(type)), commandSize(size) {
        // ...
    }

    /// Check if a command is of value type
    bool Is(CommandType type) const {
        return commandType == static_cast<uint32_t>(type);
    }

    /// Check if a command is of type
    template<typename T>
    bool Is() const {
        return commandType == static_cast<uint32_t>(T::kType);
    }

    /// Reinterpret the command type
    template<typename T>
    const T* As() const {
        ASSERT(static_cast<uint32_t>(T::kType) == commandType, "Invalid command cast");
        return static_cast<const T*>(this);
    }

    /// Cast the command
    template<typename T>
    const T* Cast() const {
        if (!Is<T>()) {
            return nullptr;
        }

        return As<T>();
    }
};

/// Command helper type
template<typename T, CommandType TYPE>
struct TCommand : public Command {
    static constexpr CommandType kType = TYPE;

    TCommand() : Command(kType, sizeof(T)) {

    }
};

struct SetShaderProgramCommand : public TCommand<SetShaderProgramCommand, CommandType::SetShaderProgram> {
    ShaderProgramID id;
};

struct SetEventDataCommand : public TCommand<SetEventDataCommand, CommandType::SetEventData> {
    ShaderDataID id;
    uint32_t value;
};

struct SetDescriptorDataCommand : public TCommand<SetEventDataCommand, CommandType::SetDescriptorData> {
    ShaderDataID id;
    uint32_t value;
};

enum class StageBufferFlag {
    None = 0,
    Atomic32 = BIT(1)
};

BIT_SET(StageBufferFlag);

struct StageBufferCommand : public TCommand<StageBufferCommand, CommandType::StageBuffer> {
    ShaderDataID id;
    size_t offset;
    StageBufferFlagSet flags;
};

struct DispatchCommand : public TCommand<DispatchCommand, CommandType::Dispatch> {
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};
