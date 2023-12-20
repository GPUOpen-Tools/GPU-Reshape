#pragma once

// Automation
#include <Test/Automation/Diagnostic/Diagnostic.h>

struct DiagnosticScope {
public:
    /// Constructor
    /// \param registry given registry to pool diagnsotics from
    /// \param condition condition in which to apply the scope
    /// \param format formatting string
    /// \param args all arguments to the formatting string
    template<typename... T>
    DiagnosticScope(Registry* registry, bool condition, const char* format, T&&... args) {
        if (condition) {
            diagnostic = registry->Get<Diagnostic>();
            diagnostic->Log(format, std::forward<T>(args)...);
            diagnostic->Indent(1);
        }
    }

    /// Constructor
    /// \param registry given registry to pool diagnsotics from
    /// \param format formatting string
    /// \param args all arguments to the formatting string
    template<typename... T>
    DiagnosticScope(Registry* registry, const char* format, T&&... args) : DiagnosticScope(registry, true, format, std::forward<T>(args)...){
    }

    /// Deconstructor
    ~DiagnosticScope() {
        if (diagnostic) {
            diagnostic->Indent(-1);
        }
    }

private:
    /// Optional diagnostics
    ComRef<Diagnostic> diagnostic;
};
