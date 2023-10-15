// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include "Kernel.h"
#include "Resource.h"

#include <vector>

struct ProgramInvocation {
    int64_t groupCountX{0};
    int64_t groupCountY{0};
    int64_t groupCountZ{0};
};

enum class MessageCheckMode {
    Generator,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual
};

struct ProgramCheck {
    std::string_view str;
};

struct Generator {
    std::string contents;
};

struct ProgramMessageAttribute {
    /// Name of the attribute
    std::string_view name;

    /// Expected value generator
    Generator checkGenerator{"1"};
};

struct ProgramMessage {
    /// Schema type of the message
    std::string_view type;

    /// Message check generator
    Generator checkGenerator{"1"};

    /// Line in which it occurs
    int64_t line{0};

    /// All attributes
    std::vector<ProgramMessageAttribute> attributes;
};

struct Program {
    /// Kernel info
    Kernel kernel;

    /// Optional executor
    std::string_view executor;

    /// Is this program safe guarded?
    bool isSafeGuarded{false};

    /// Expected invocations
    std::vector<ProgramInvocation> invocations;

    /// All schemas to include
    std::vector<std::string_view> schemas;

    /// All resources to generate
    std::vector<Resource> resources;

    /// All expected checks
    std::vector<ProgramCheck> checks;

    /// All expected messages
    std::vector<ProgramMessage> messages;
};
