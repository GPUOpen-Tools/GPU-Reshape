#pragma once

// Common
#include <Common/Assert.h>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/Emitter.h>

class SPIRVModule {
public:
    /// Parse a module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    bool ParseModule(const uint32_t* code, uint32_t wordCount);

    /// Get the produced program
    /// \return
    const IL::Program& GetProgram() const {
        return program;
    }

private:
    /// Parsing context
    struct ParseContext {
        uint32_t operator*() const {
            ASSERT(code < end, "End of stream");
            return *code;
        }

        uint32_t operator++() {
            ASSERT(code < end, "End of stream");
            return *(++code);
        }

        uint32_t operator++(int) {
            ASSERT(code < end, "End of stream");
            return *(code++);
        }

        uint32_t WordCount() const {
            return end - code;
        }

        uint32_t Good() const {
            return end > code;
        }

        /// Code pointers
        const uint32_t* code;
        const uint32_t* end;

        /// Current function
        IL::Function* function{nullptr};

        /// Emitter, holds current basic block
        IL::Emitter emitter;
    };

    /// Parse the header
    /// \param context
    /// \return
    bool ParseHeader(ParseContext& context);

    /// Parse an instruction
    /// \param context
    /// \return
    bool ParseInstruction(ParseContext& context);

private:
    /// Header specification
    struct ProgramHeader {
        uint32_t version;
        uint32_t generator;
        uint32_t bound;
        uint32_t reserved;
    };

    /// Header
    ProgramHeader header;

private:
    /// Abstracted program
    IL::Program program;
};
