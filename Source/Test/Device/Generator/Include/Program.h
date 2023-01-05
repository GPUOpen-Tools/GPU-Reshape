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

struct ProgramMessageAttribute {
    /// Name of the attribute
    std::string_view name;

    // Expected value of the attribute
    int64_t value;
};

struct Generator {
    std::string contents;
};

struct ProgramMessage {
    /// Schema type of the message
    std::string_view type;

    /// Message check generator
    Generator checkGenerator{"1"};

    /// Message check mode
    MessageCheckMode checkMode{MessageCheckMode::Equal};

    /// Line in which it occurs
    int64_t line{0};

    /// All attributes
    std::vector<ProgramMessageAttribute> attributes;
};

struct Program {
    /// Kernel info
    Kernel kernel;

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
