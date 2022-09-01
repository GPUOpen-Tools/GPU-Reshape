#pragma once

// Layer
#include "DXSourceAssociation.h"

// Std
#include <string_view>

class IDXDebugModule {
public:
    /// Get the source association from a given code offset
    /// \param codeOffset the instruction (i.e. record) code offset
    /// \return default if failed
    virtual DXSourceAssociation GetSourceAssociation(uint32_t codeOffset) = 0;

    /// Get a source view of a line
    /// \param fileUID originating file uid
    /// \param line originating line number
    /// \return empty if failed
    virtual std::string_view GetLine(uint32_t fileUID, uint32_t line) = 0;
};
