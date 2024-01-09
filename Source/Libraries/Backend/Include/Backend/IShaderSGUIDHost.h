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

// Backend
#include <Backend/ShaderExport.h>
#include <Backend/ShaderSourceMapping.h>
#include <Backend/IL/BasicBlock.h>

// Common
#include <Common/IComponent.h>

// Std
#include <string_view>

// Forward declarations
namespace IL {
    struct Program;
}

class IShaderSGUIDHost : public TComponent<IShaderSGUIDHost> {
public:
    COMPONENT(IShaderSGUIDHost);

    /// Bind an instruction to the sguid host
    /// \param program the program of [instruction]
    /// \param instruction the instruction to be bound
    /// \return the shader guid value
    virtual ShaderSGUID Bind(const IL::Program& program, const IL::BasicBlock::ConstIterator& instruction) = 0;

    /// Get the mapping for a given sguid
    /// \param sguid
    /// \return
    virtual ShaderSourceMapping GetMapping(ShaderSGUID sguid) = 0;

    /// Get the source code for a binding
    /// \param sguid the shader sguid
    /// \return the source code, empty if not found
    virtual std::string_view GetSource(ShaderSGUID sguid) = 0;

    /// Get the source code for a mapping
    /// \param mapping a mapping allocated from ::Bind
    /// \return the source code, empty if not found
    virtual std::string_view GetSource(const ShaderSourceMapping& mapping) = 0;
};
