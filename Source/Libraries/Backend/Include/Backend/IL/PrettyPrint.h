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
    void PrettyPrintBlockJsonGraph(const Function& function, PrettyPrintContext out);
    void PrettyPrintProgramJson(const Program& program, PrettyPrintContext out);
}
