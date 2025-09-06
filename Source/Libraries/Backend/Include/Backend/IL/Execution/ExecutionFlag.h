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

// Common
#include <Common/Enum.h>

enum class ExecutionFlag : uint32_t {
    /// This is a general draw execution
    TypeDraw = BIT(0),

    /// This is a general dispatch execution
    TypeDispatch = BIT(1),

    /// This is a general raytracing execution
    TypeRaytracing = BIT(2),

    /// The execution is indirect in nature
    TypeIndirect = BIT(3)
};

enum class ExecutionDrawFlag : uint32_t {
    VertexCountPerInstance = BIT(0),
    IndexCountPerInstance = BIT(1),
    InstanceCount = BIT(2),
    StartVertex = BIT(3),
    StartIndex = BIT(4),
    StartInstance = BIT(5),
    VertexOffset = BIT(6),
    InstanceOffset = BIT(7)
};

BIT_SET(ExecutionFlag);
BIT_SET(ExecutionDrawFlag);
