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

#include <Parser.h>
#include <iostream>
#include <string>

Parser::Parser(Program &program) : program(program) {

}

bool Parser::Parse(const char *code) {
    // Tokenize the stream
    if (!Tokenize(code)) {
        return false;
    }

    for (const TokenBucket& bucket : buckets) {
        Context context(bucket.tokens);

        // Try to parse as statement
        if (!ParseStatement(context)) {
            return false;
        }

        // Must be end of statement
        if (!context.IsEOS()) {
            context.Error("Expected end of statement");
            return false;
        }
    }

    // OK
    return true;
}

bool Parser::Tokenize(const char *code) {
    constexpr const char* kStatement = "//!";

    // Current line
    uint32_t line{1};

    // Note: Slightly naive implementation, but sufficient for now

    // EOS?
    while (*code) {
        // Eat whitespaces at start of line
        while (*code && *code != '\n' && std::isspace(*code))
            code++;

        // Statement?
        if (!std::strncmp(code, kStatement, std::strlen(kStatement))) {
            code += std::strlen(kStatement);

            // Create new bucket
            TokenBucket& bucket = buckets.emplace_back();

            // Parse until end of line
            while (*code && *code != '\n') {
                // Skip whitespaces
                while (*code && *code != '\n' && std::isspace(*code))
                    code++;

                // End?
                if (*code == '\n')
                    break;

                // Alpha identifier?
                if (isalpha(*code)) {
                    const char* start{code};

                    while (*code && isalnum(*code)) {
                        code++;
                    }

                    Token token;
                    token.type = TokenType::ID;
                    token.str = std::string_view (start, code - start);
                    token.line = line;
                    bucket.tokens.push_back(token);
                }

                // String?
                else if (*code == '"') {
                    code++;

                    // Read until end of string
                    const char* start{code};
                    while (*code && *code != '\n' && *code != '\"')
                        code++;

                    // Must be end of string (could be EOS, EOL)
                    if (*code != '\"') {
                        std::cerr << "Line " << line << ", expected end of string" << std::endl;
                        return false;
                    }

                    code++;

                    Token token;
                    token.type = TokenType::String;
                    token.str = std::string_view (start, code - start - 1);
                    token.line = line;
                    bucket.tokens.push_back(token);
                }

                // Generator?
                else if (*code == '{') {
                    code++;

                    // Read until end of string
                    const char* start{code};
                    while (*code && *code != '\n' && *code != '}')
                        code++;

                    // Must be end of string (could be EOS, EOL)
                    if (*code != '}') {
                        std::cerr << "Line " << line << ", expected end of string" << std::endl;
                        return false;
                    }

                    code++;

                    Token token;
                    token.type = TokenType::Generator;
                    token.str = std::string_view (start, code - start - 1);
                    token.line = line;
                    bucket.tokens.push_back(token);
                }

                // Numeric?
                else if (isdigit(*code)) {
                    const char* start{code};
                    bool isFp{false};

                    // Naive parsing of int or fp
                    while (*code && (isdigit(*code) || *code == '.')) {
                        code++;
                        isFp |= *code == '.';
                    }

                    // Floating point?
                    if (*code == 'f' || isFp) {
                        Token token;
                        token.type = TokenType::Float;
                        token.line = line;

                        // Attempt to convert
                        char* end;
                        token.fp64 = std::strtod(start, &end);

                        // Failed?
                        if (!end) {
                            std::cerr << "Line " << line << ", invalid floating point number" << std::endl;
                            return false;
                        }

                        bucket.tokens.push_back(token);
                        code++;
                    } else {
                        Token token;
                        token.type = TokenType::Int;
                        token.line = line;

                        // Attempt to convert
                        char* end;
                        token.int64 = std::strtoll(start, &end, 10);

                        // Failed?
                        if (!end) {
                            std::cerr << "Line " << line << ", invalid integer number" << std::endl;
                            return false;
                        }

                        bucket.tokens.push_back(token);
                    }
                } else {
                    Token token;
                    token.type = TokenType::Symbol;
                    token.str = std::string_view (code, 1);
                    token.line = line;
                    bucket.tokens.push_back(token);

                    code++;
                }
            }
        }

        // Eat rest of line
        while (*code && *code != '\n')
            code++;

        // Next line
        if (*code) {
            code++;
            line++;
        }
    }

    return true;
}

bool Parser::ParseStatement(Parser::Context &context) {
    // Handle type
    if (context.TryNext("KERNEL")) {
        return ParseKernel(context);
    } else if (context.TryNext("DISPATCH")) {
        return ParseDispatch(context);
    } else if (context.TryNext("RESOURCE")) {
        return ParseResource(context);
    } else if (context.TryNext("CHECK")) {
        return ParseCheck(context);
    } else if (context.TryNext("MESSAGE")) {
        return ParseMessage(context);
    } else if (context.TryNext("SCHEMA")) {
        return ParseSchema(context);
    } else if (context.TryNext("EXECUTOR")) {
        return ParseExecutor(context);
    }else if (context.TryNext("SAFEGUARD")) {
        return ParseSafeGuard(context);
    }

    // Unknown
    context.Error("Unknown test statement ");
    return false;
}

bool Parser::ParseKernel(Parser::Context &context) {
    // Parse type
    if (!ParseKernelType(context, &program.kernel.type)) {
        return false;
    }

    // Parse name
    if (!ParseString(context, &program.kernel.name)) {
        return false;
    }

    // OK
    return true;
}

bool Parser::ParseSchema(Parser::Context &context) {
    std::string_view& schema = program.schemas.emplace_back();

    // Parse name
    if (!ParseString(context, &schema)) {
        return false;
    }

    // OK
    return true;
}

bool Parser::ParseExecutor(Parser::Context &context) {
    // Parse executor
    return ParseString(context, &program.executor);
}

bool Parser::ParseSafeGuard(Parser::Context &context) {
    program.isSafeGuarded = true;
    return true;
}

bool Parser::ParseDispatch(Parser::Context &context) {
    ProgramInvocation& invocation = program.invocations.emplace_back();

    if (!ParseInt(context, &invocation.groupCountX)) {
        return false;
    }

    if (!context.TryNext(",")) {
        context.Error("Expected ,");
        return false;
    }

    if (!ParseInt(context, &invocation.groupCountY)) {
        return false;
    }

    if (!context.TryNext(",")) {
        context.Error("Expected ,");
        return false;
    }

    if (!ParseInt(context, &invocation.groupCountZ)) {
        return false;
    }

    return true;
}

bool Parser::ParseResource(Parser::Context &context) {
    Resource& resource = program.resources.emplace_back();

    // Parse the type
    if (!ParseResourceType(context, &resource.type)) {
        return false;
    }

    if (context.TryNext("<")) {
        Token format = context.Next();

        if (format.type != TokenType::ID) {
            context.Error("Expected format identifier");
            return false;
        }

        resource.format = format.str;

        if (!context.TryNext(">")) {
            context.Error("Expected end of format template");
            return false;
        }
    }

    if (context.TryNext("[")) {
        Token count = context.Next();

        if (count.type != TokenType::Int) {
            context.Error("Expected size integer");
            return false;
        }

        resource.format = count.str;

        if (!context.TryNext("]")) {
            context.Error("Expected end of array size");
            return false;
        }
    }

    while (context) {
        Token attribute = context.Next();

        // Must be ID
        if (attribute.type != TokenType::ID) {
            context.Error("Expected attribute");
            return false;
        }

        // Expecting a:b
        if (!context.TryNext(":")) {
            context.Error("Expected : between attribute and value");
            return false;
        }

        // Handle type
        if (attribute == "size") {
            while (context) {
                int64_t value;
                if (!ParseInt(context, &value)) {
                    return false;
                }

                resource.initialization.sizes.push_back(value);

                if (!context.Is(",")) {
                    break;
                }

                context.Next();
            }
        } else if (attribute == "data") {
            while (context) {
                int64_t value;
                if (!ParseInt(context, &value)) {
                    return false;
                }

                resource.initialization.data.push_back(value);

                if (!context.Is(",")) {
                    break;
                }

                context.Next();
            }
        } else if (attribute == "width") {
            while (context) {
                int64_t value;
                if (!ParseInt(context, &value)) {
                    return false;
                }

                resource.structuredSize = static_cast<uint32_t>(value);

                if (!context.Is(",")) {
                    break;
                }

                context.Next();
            }
        } else {
            context.Error("Unknown attribute type");
            return false;
        }
    }

    return true;
}

bool Parser::ParseCheck(Parser::Context &context) {
    ProgramCheck& check = program.checks.emplace_back();

    // Parse the check string
    if (!ParseString(context, &check.str)) {
        return false;
    }

    return true;
}

bool Parser::ParseMessage(Parser::Context &context) {
    ProgramMessage& message = program.messages.emplace_back();
    message.line = context.Tok().line;

    // Must be ID
    Token type = context.Next();
    if (type.type != TokenType::ID) {
        context.Error("Expected message name");
        return false;
    }

    message.type = type.str;

    if (!context.TryNext("[")) {
        context.Error("Expected start of count [");
        return false;
    }

    if (!ParseLiteralGenerator(context, &message.checkGenerator)) {
        return false;
    }

    if (!context.TryNext("]")) {
        context.Error("Expected start of count ]");
        return false;
    }

    while (context) {
        ProgramMessageAttribute& attr = message.attributes.emplace_back();

        // Must be ID
        Token attribute = context.Next();
        if (attribute.type != TokenType::ID) {
            context.Error("Expected attribute");
            return false;
        }

        attr.name = attribute.str;

        // Expecting a:b
        if (!context.TryNext(":")) {
            context.Error("Expected : between attribute and value");
            return false;
        }

        if (!ParseLiteralGenerator(context, &attr.checkGenerator)) {
            return false;
        }
    }

    return true;
}

bool Parser::ParseKernelType(Parser::Context &context, KernelType* out) {
    // Parse type
    if (context.TryNext("Compute")) {
        *out = KernelType::Compute;
    } else {
        context.Error("Unknown kernel type");
        return false;
    }

    // OK
    return true;
}

bool Parser::ParseString(Parser::Context &context, std::string_view *out) {
    if (context.Tok().type != TokenType::String) {
        context.Error("Expected string");
        return false;
    }

    *out = context.Next().str;
    return true;
}

bool Parser::ParseInt(Parser::Context &context, int64_t* out) {
    if (context.Tok().type != TokenType::Int) {
        context.Error("Expected integer");
        return false;
    }

    *out = context.Next().int64;
    return true;
}

bool Parser::ParseResourceType(Parser::Context &context, ResourceType *out) {
    // Parse type
    if (context.TryNext("Buffer")) {
        *out = ResourceType::Buffer;
    } else if (context.TryNext("RWBuffer")) {
        *out = ResourceType::RWBuffer;
    } else if (context.TryNext("StructuredBuffer")) {
        *out = ResourceType::StructuredBuffer;
    } else if (context.TryNext("RWStructuredBuffer")) {
        *out = ResourceType::RWStructuredBuffer;
    } else if (context.TryNext("Texture1D")) {
        *out = ResourceType::Texture1D;
    } else if (context.TryNext("RWTexture1D")) {
        *out = ResourceType::RWTexture1D;
    } else if (context.TryNext("Texture2D")) {
        *out = ResourceType::Texture2D;
    } else if (context.TryNext("RWTexture2D")) {
        *out = ResourceType::RWTexture2D;
    } else if (context.TryNext("RWTexture2DArray")) {
        *out = ResourceType::RWTexture2DArray;
    } else if (context.TryNext("Texture3D")) {
        *out = ResourceType::Texture3D;
    } else if (context.TryNext("RWTexture3D")) {
        *out = ResourceType::RWTexture3D;
    } else if (context.TryNext("SamplerState")) {
        *out = ResourceType::SamplerState;
    } else if (context.TryNext("StaticSamplerState")) {
        *out = ResourceType::StaticSamplerState;
    } else if (context.TryNext("CBuffer")) {
        *out = ResourceType::CBuffer;
    } else {
        context.Error("Unknown resource type");
        return false;
    }

    // OK
    return true;
}

bool Parser::ParseLiteralGenerator(Parser::Context &context, Generator *out) {
    MessageCheckMode checkMode = MessageCheckMode::Equal;
    
    // Optional comparison mode
    if (context.TryNext("==")) {
        checkMode = MessageCheckMode::Equal;
    } else if (context.TryNext("!=")) {
        checkMode = MessageCheckMode::NotEqual;
    } else if (context.TryNext(">")) {
        checkMode = MessageCheckMode::Greater;
    } else if (context.TryNext(">=")) {
        checkMode = MessageCheckMode::GreaterEqual;
    } else if (context.TryNext("<")) {
        checkMode = MessageCheckMode::Less;
    } else if (context.TryNext("<=")) {
        checkMode = MessageCheckMode::LessEqual;
    }

    // Generator is pass through
    if (context.Tok().type == TokenType::Generator) {
        checkMode = MessageCheckMode::Generator;
        out->contents = context.Next().str;
    } else {
        // Parse as integer
        int64_t value;
        if (!ParseInt(context, &value)) {
            return false;
        }

        switch (checkMode) {
            default:
            ASSERT(false, "Invalid check mode");
            case MessageCheckMode::Equal:
                out->contents = "x == " + std::to_string(value);
                break;
            case MessageCheckMode::NotEqual:
                out->contents = "x != " + std::to_string(value);
                break;
            case MessageCheckMode::Greater:
                out->contents = "x > " + std::to_string(value);
                break;
            case MessageCheckMode::GreaterEqual:
                out->contents = "x >= " + std::to_string(value);
                break;
            case MessageCheckMode::Less:
                out->contents = "x < " + std::to_string(value);
                break;
            case MessageCheckMode::LessEqual:
                out->contents = "x <= " + std::to_string(value);
                break;
        }
    }

    // OK
    return true;
}

void Parser::Context::Error(const char *message) {
    std::cerr << "Line " << Tok().line << ", " << message << std::endl;
}
