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

struct SetDescriptorDataCommand : public TCommand<SetDescriptorDataCommand, CommandType::SetDescriptorData> {
    ShaderDataID id;
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

struct ClearBufferCommand : public TCommand<ClearBufferCommand, CommandType::ClearBuffer> {
    ShaderDataID id;
    size_t offset;
    size_t length;
    uint32_t value;
};

struct DiscardCommand : public TCommand<DiscardCommand, CommandType::Discard> {
    uint32_t puid;
};

struct DispatchCommand : public TCommand<DispatchCommand, CommandType::Dispatch> {
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};

struct UAVBarrierCommand : public TCommand<UAVBarrierCommand, CommandType::UAVBarrier> {
    
};
