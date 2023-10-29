#pragma once

// Layer
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticMessage.h>

// Std
#include <sstream>

/// Pretty print a diagnostic message
/// \param message given message
/// \param out destination stream, written to current offset
void DiagnosticPrettyPrint(const DiagnosticMessage<DiagnosticType>& message, std::stringstream& out);
