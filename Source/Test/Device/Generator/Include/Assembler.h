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

// Generator
#include "Program.h"

// Common
#include <Common/TemplateEngine.h>

// Std
#include <map>
#include <vector>

/// Assembly info
struct AssembleInfo {
    /// Common paths
    const char* templatePath{nullptr};
    const char* shaderPath{nullptr};

    /// Names
    const char* program{nullptr};
    const char* feature{nullptr};
};

class Assembler {
public:
    Assembler(const AssembleInfo& info, const Program& program);

    /// Assemble to a given stream
    /// \param out the output stream
    /// \return success state
    bool Assemble(std::ostream& out);

private:
    void AssembleConstraints();
    void AssembleResources();

private:
    const Program& program;

    /// Assembly info
    AssembleInfo assembleInfo;

    /// Bucket by message type
    std::map<std::string_view, std::vector<ProgramMessage>> buckets;

    /// Engines
    TemplateEngine testTemplate;
    TemplateEngine constraintTemplate;
};