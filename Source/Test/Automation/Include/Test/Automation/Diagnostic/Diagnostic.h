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

// Common
#include <Common/Registry.h>
#include <Common/Format.h>

// Std
#include <chrono>
#include <iostream>

class Diagnostic : public TComponent<Diagnostic> {
public:
    COMPONENT(TestDiagnostic);

    /// Log a message
    /// \param format formatting string
    /// \param args all arguments for formatting
    template<typename... T>
    void Log(const char* format, T&&... args) {
        // Output with time stamp and indentation
        // TODO: Time zoning!
        std::cout
            << std::format("{:%H:%M:%OS}\t", std::chrono::system_clock::now())
            << std::string(static_cast<uint32_t>(indent * 2u), ' ')
            << Format(format, std::forward<T>(args)...) << std::endl;
    }

    /// Indent this diagnostic
    /// \param level given level, must not underflow
    void Indent(int32_t level) {
        ASSERT(level >= 0 || indent >= level, "Invalid level");
        indent += level;
    }

    /// Allocate a new counter, opaque in nature
    /// Useful for "unique" identifiers across tests
    uint64_t AllocateCounter() {
        return counter++;
    }

private:
    /// Current indentation level
    int32_t indent = 0;

    /// Current counter
    uint64_t counter = 0;
};

/// Log a message
/// \param registry registry to pool diagnostics from
/// \param format formatting string
/// \param args all arguments for formatting
template<typename... T>
inline void Log(Registry* registry, const char* format, T&&... args) {
    if (ComRef ref = registry->Get<Diagnostic>()) {
        ref->Log(format, std::forward<T>(args)...);
    }
}
