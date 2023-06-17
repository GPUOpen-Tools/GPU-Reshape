// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Layer
#include "DXSourceAssociation.h"

// Std
#include <string_view>

class IDXDebugModule {
public:
    virtual ~IDXDebugModule() = default;

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

    /// Get the source filename
    /// \return filename
    virtual std::string_view GetSourceFilename(uint32_t fileUID) = 0;

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
