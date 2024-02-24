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

enum class BasicBlockFlag : uint32_t {
    None = 0x0,

    /// Do not perform instrumentation on this block
    NoInstrumentation = BIT(1),

    /// Block has been visited
    Visited = BIT(2),

    /// Set of mutable flags, these flags may be changed on immutable blocks
    /// There are no semantic implications
    NonSemanticMask = Visited
};

BIT_SET(BasicBlockFlag);

enum class BasicBlockSplitFlag : uint32_t {
    None = 0x0,

    /// Perform patching on all branch users past the split point to the new block
    RedirectBranchUsers = BIT(1),

    /// All of the above
    RedirectAll = RedirectBranchUsers,
};

BIT_SET(BasicBlockSplitFlag);
