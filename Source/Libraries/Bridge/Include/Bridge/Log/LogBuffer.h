// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Bridge
#include "LogSeverity.h"

// Message
#include <Message/MessageStream.h>

// Std
#include <mutex>
#include <string_view>

// Forward declarations
class IBridge;

class LogBuffer {
public:
    /// Add a new message
    ///   ? For formatting see LogFormat.h, separated for compile times
    /// \param system responsible system
    /// \param severity message severity
    /// \param message message contents
    void Add(const std::string_view& system, LogSeverity severity, const std::string_view& message);

    /// Commit all messages
    /// \param bridge
    void Commit(IBridge* bridge);

private:
    /// Shared mutex
    std::mutex mutex;

    /// Underlying stream
    MessageStream stream;
};
