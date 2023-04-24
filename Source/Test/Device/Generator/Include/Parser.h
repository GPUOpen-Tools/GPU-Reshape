#pragma once

// Common
#include <Common/Assert.h>

// Std
#include <string_view>
#include <vector>
#include <cctype>

// Generator
#include "Program.h"

class Parser {
public:
    Parser(Program& program);

    /// Parse a given program
    /// \param code the program to be parsed
    /// \return success state
    bool Parse(const char *code);

private:
    /// Type of a single token
    enum class TokenType {
        None,
        ID,
        String,
        Generator,
        Symbol,
        Int,
        Float
    };

    /// Single token
    struct Token {
        Token() {

        }

        /// Equality
        bool operator==(const std::string_view& rhs) const {
            return (type == TokenType::ID || type == TokenType::String || type == TokenType::Symbol) && str == rhs;
        }

        /// Line of this token
        uint32_t line{0};

        /// Type of the token
        TokenType type{TokenType::None};

        /// Data payload
        union {
            std::string_view str;
            int64_t int64;
            double fp64;
        };
    };

    /// Single segment of tokens
    struct TokenBucket {
        std::vector<Token> tokens;
    };

    /// Tokenize a string
    /// \param code the string to be tokenized
    /// \return success state
    bool Tokenize(const char* code);

private:
    /// Parsing context
    struct Context {
        Context(const std::vector<Token>& tokens) : it(tokens.begin()), end(tokens.end()) {

        }

        /// Get the current token
        Token Tok() const {
            return it == end ? Token{} : *it;
        }

        /// Get the next token
        Token Next() {
            if (it != end) {
                return *it++;
            }
            return {};
        }

        /// Check if the current token is a given string
        bool Is(const std::string_view& str) const {
            return Tok() == str;
        }

        /// Advance a token if it is a given string
        /// \return true if advanced
        bool TryNext(const std::string_view& str) {
            if (Is(str)) {
                Next();
                return true;
            }
            return false;
        }

        /// End of stream?
        bool IsEOS() const {
            return it == end;
        }

        /// Good state?
        operator bool() const {
            return it != end;
        }

        /// Report error for current token
        void Error(const char* message);

        std::vector<Token>::const_iterator it;
        std::vector<Token>::const_iterator end;
    };

    /// Parsers
    bool ParseStatement(Context& context);
    bool ParseKernel(Context& context);
    bool ParseDispatch(Context& context);
    bool ParseResource(Context& context);
    bool ParseCheck(Context& context);
    bool ParseMessage(Context& context);
    bool ParseSchema(Context& context);
    bool ParseExecutor(Context& context);
    bool ParseSafeGuard(Context& context);
    bool ParseKernelType(Context& context, KernelType* out);
    bool ParseResourceType(Context& context, ResourceType* out);
    bool ParseString(Context& context, std::string_view* out);
    bool ParseInt(Context& context, int64_t* out);

    /// Generator helper
    bool ParseLiteralGenerator(Context& context, Generator* out);

private:
    Program& program;

    /// All buckets
    std::vector<TokenBucket> buckets;
};
