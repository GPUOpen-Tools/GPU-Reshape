﻿// 
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

#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticPrettyPrint.h>
#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticView.h>

void DiagnosticPrettyPrint(const DiagnosticMessage<DiagnosticType> &message, std::stringstream &out) {
    DiagnosticView view(message);

    switch (message.type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case DiagnosticType::ShaderParsingFailed:
            out << "Shader " << view.Get<uint64_t>() << " - Parsing failed";
            break;
        case DiagnosticType::ShaderInternalCompilerError:
            out << "Shader " << view.Get<uint64_t>() << " - Internal compiler error";
            break;
        case DiagnosticType::ShaderCreationFailed:
            out << "Shader " << view.Get<uint64_t>() << " - Driver creation failed";
            break;
        case DiagnosticType::PipelineMissingShaderKey:
            out << "Pipeline " << view.Get<uint64_t>() << " - Missing shader stage key";
            break;
        case DiagnosticType::PipelineCreationFailed:
            out << "Pipeline " << view.Get<uint64_t>() << " - Driver creation failed";
            break;
    }
}
