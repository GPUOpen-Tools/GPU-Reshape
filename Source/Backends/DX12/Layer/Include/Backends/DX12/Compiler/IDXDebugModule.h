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

    /// Get the primary filename
    /// \return filename
    virtual std::string_view GetFilename() = 0;

    /// Get the number of files
    /// \return count
    virtual uint32_t GetFileCount() = 0;

    /// Get the total size of the combined source code
    /// \return length, not null terminated
    virtual uint64_t GetCombinedSourceLength(uint32_t fileUID) const = 0;

    /// Fill the combined source code into an output buffer
    /// \param buffer length must be at least GetCombinedSourceLength
    virtual void FillCombinedSource(uint32_t fileUID, char* buffer) const = 0;
};
