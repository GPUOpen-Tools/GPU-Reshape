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
