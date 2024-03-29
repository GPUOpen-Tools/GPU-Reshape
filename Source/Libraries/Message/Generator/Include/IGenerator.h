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

// Generator
#include "Schema.h"
#include "Language.h"

// Common
#include <Common/IComponent.h>

// Std
#include <sstream>

/// Schema output stream
struct SchemaStream {
    std::stringstream& header;
    std::stringstream& footer;
};

/// Message output stream
struct MessageStream {
    SchemaStream schema;

    std::stringstream& header;
    std::stringstream& footer;
    std::stringstream& schemaType;
    std::stringstream& chunks;
    std::stringstream& types;
    std::stringstream& functions;
    std::stringstream& members;

    std::string base;
    uint32_t size{0};
};

/// Schema and message generator
class IGenerator : public TComponent<IGenerator> {
public:
    COMPONENT(IGenerator);

    /// Generate the schema
    /// \param schema the schema tree
    /// \param language the target language
    /// \param out the output schema stream
    /// \return success state
    virtual bool Generate(Schema &schema, Language language, SchemaStream& out) = 0;

    /// Generate a message
    /// \param message the message being generated
    /// \param language the target language
    /// \param out the output message stream
    /// \return success state
    virtual bool Generate(const Message& message, Language language, MessageStream& out) = 0;
};
