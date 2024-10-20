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
#include "FeatureHookTable.h"

// Common
#include <Common/IComponent.h>
#include <Message/MessageStream.h>

// Forward declarations
struct MessageStream;

// IL
namespace IL {
    struct Program;
}

class IShaderFeature : public IInterface {
public:
    COMPONENT(IShaderFeature);

    /// Collect all shader exports
    /// \param stream the produced stream
    virtual void CollectExports(const MessageStream& stream) { /* no collection */};

    /// Invoked before injection takes place
    /// \param program the program to be injected    
    virtual void PreInject(IL::Program &program, const MessageStreamView<> &specialization) { /* no pre-injection */};

    /// Perform injection into a program
    /// \param program the program to be injected to
    virtual void Inject(IL::Program &program, const MessageStreamView<> &specialization) { /* no injection */ }
};
