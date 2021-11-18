#pragma once

// Common
#include <Common/Assert.h>

// Backend
#include <Backend/IL/Program.h>
#include <Backend/IL/Emitter.h>

class SPIRVModule {
public:
    SPIRVModule(const Allocators& allocators);
    ~SPIRVModule();

    /// No copy
    SPIRVModule(const SPIRVModule& other) = delete;
    SPIRVModule& operator=(const SPIRVModule& other) = delete;

    /// No move
    SPIRVModule(SPIRVModule&& other) = delete;
    SPIRVModule& operator=(SPIRVModule&& other) = delete;

    /// Copy this module
    /// \return
    SPIRVModule* Copy() const;

    /// Parse a module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    bool ParseModule(const uint32_t* code, uint32_t wordCount);

    /// Recompile the program, code must be the same as the originally parsed module
    /// \param code the SPIRV module pointer
    /// \param wordCount number of words within the module stream
    /// \return success state
    bool Recompile(const uint32_t* code, uint32_t wordCount);

    /// Get the produced program
    /// \return
    const IL::Program* GetProgram() const {
        return program;
    }

    /// Get the produced program
    /// \return
    IL::Program* GetProgram() {
        return program;
    }

    /// Get the code pointer
    const uint32_t* GetCode() const {
        return spirvProgram.data();
    }

    /// Get the byte size of the code
    uint64_t GetSize() const {
        return spirvProgram.size() * sizeof(uint32_t);
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
        IL::Emitter<> emitter;
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
    Allocators allocators;

    /// JIT'ed program
    std::vector<uint32_t> spirvProgram;

    /// Abstracted program
    IL::Program* program{nullptr};
};
