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

#pragma once

// Address
#include <Addressing/IL/TexelAddress.h>
#include <Addressing/Config.h>

// Backend
#include <Backend/IL/ID.h>

struct TexelProperties {
    /// Address of the texel
    TexelAddress<IL::ID> address;
    
    /// Source view coordinates
    IL::ID x{};
    IL::ID y{};
    IL::ID z{};
    IL::ID mip{};
    IL::ID offset{};

    /// The value type texel count
    /// This may not represent the runtime width
    uint32_t texelCountLiteral{};

    /// The packed token of the owning resource
    IL::ID packedToken{IL::InvalidID};

    /// Assigned PUID of the owning resource
    IL::ID puid{IL::InvalidID};

    /// The memory base offset of the initialization masks
    IL::ID texelBaseOffsetAlign32{IL::InvalidID};

    /// Failure block code
    IL::ID failureBlock{IL::InvalidID};

#if TEXEL_ADDRESSING_ENABLE_FENCING
    /// Total number of texels in the resource
    IL::ID resourceTexelCount{IL::InvalidID};

    /// True if invalid addressing (i.e. out of bounds)
    IL::ID invalidAddressing{IL::InvalidID};
#endif // TEXEL_ADDRESSING_ENABLE_FENCING

    /// Debugging value
    IL::ID debug{};
};
