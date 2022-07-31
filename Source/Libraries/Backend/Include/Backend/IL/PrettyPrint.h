#pragma once

// Std
#include <ostream>

// Backend
#include "Format.h"
#include "ResourceSamplerMode.h"

namespace Backend::IL {
    struct ConstantMap;
    struct TypeMap;
    struct Type;
}

namespace IL {
    struct Program;
    struct Function;
    struct BasicBlock;
    struct Instruction;
    struct SOVValue;

    /// Pretty printing context, holds printing streams and padding requirements
    struct PrettyPrintContext {
        PrettyPrintContext(std::ostream& stream) : stream(stream) {

        }

        /// Tab this context
        void TabInline() {
            pad++;
        }

        /// Insert a tab into the current padding
        /// \return the new context
        PrettyPrintContext Tab() const {
            PrettyPrintContext ctx(stream);
            ctx.pad = pad + 1;
            return ctx;
        }

        /// Start a new line
        /// \return the stream, must be terminated with end-of-line
        std::ostream& Line() {
            for (uint32_t i = 0; i < pad; i++) {
                stream << "\t";
            }

            return stream;
        }

        /// Output stream
        std::ostream& stream;

        /// Current padding
        uint32_t pad{0};
    };

    /// Pretty printers
    void PrettyPrint(const Program& program, PrettyPrintContext out);
    void PrettyPrint(const Function& function, PrettyPrintContext out);
    void PrettyPrint(const BasicBlock& basicBlock, PrettyPrintContext out);
    void PrettyPrint(const Instruction* instr, PrettyPrintContext out);
    void PrettyPrint(const SOVValue& value, PrettyPrintContext out);
    void PrettyPrint(Backend::IL::Format format, PrettyPrintContext out);
    void PrettyPrint(Backend::IL::ResourceSamplerMode mode, PrettyPrintContext out);
    void PrettyPrint(const Backend::IL::Type* map, PrettyPrintContext out);
    void PrettyPrint(const Backend::IL::TypeMap& map, PrettyPrintContext out);
    void PrettyPrint(const Backend::IL::ConstantMap& map, PrettyPrintContext out);
    void PrettyPrintBlockDotGraph(const Function& function, PrettyPrintContext out);
}
