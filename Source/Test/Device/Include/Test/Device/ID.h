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

#include <Common/UniqueID.h>

namespace Test {
    UNIQUE_ID(uint32_t, ResourceID);
    UNIQUE_ID(uint32_t, ResourceLayoutID);
    UNIQUE_ID(uint32_t, ResourceSetID);
    UNIQUE_ID(uint32_t, QueueID);
    UNIQUE_ID(uint32_t, CommandBufferID);
    UNIQUE_ID(uint32_t, PipelineID);

    UNIQUE_ID(ResourceID, BufferID);
    UNIQUE_ID(ResourceID, TextureID);
    UNIQUE_ID(ResourceID, SamplerID);
    UNIQUE_ID(ResourceID, CBufferID);
}
