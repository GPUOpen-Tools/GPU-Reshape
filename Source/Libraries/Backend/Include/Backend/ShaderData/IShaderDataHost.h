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
#include "ShaderData.h"
#include "ShaderDataBufferInfo.h"
#include "ShaderDataEventInfo.h"
#include "ShaderDataDescriptorInfo.h"
#include "ShaderDataInfo.h"

// Common
#include <Common/IComponent.h>

class IShaderDataHost : public TComponent<IShaderDataHost> {
public:
    COMPONENT(IShaderDataHost);

    /// Create a new buffer
    /// \param info buffer information
    /// \return invalid if failed
    virtual ShaderDataID CreateBuffer(const ShaderDataBufferInfo& info) = 0;

    /// Create a new event data
    /// \param info buffer information
    /// \return invalid if failed
    virtual ShaderDataID CreateEventData(const ShaderDataEventInfo& info) = 0;

    /// Create a new descriptor data
    /// \param info descriptor data information
    /// \return invalid if failed
    virtual ShaderDataID CreateDescriptorData(const ShaderDataDescriptorInfo& info) = 0;

    /// Create a new mapping, used for tile updates
    /// \param data the data, or one of same creation parameters, to be mapped to
    /// \param tileCount number of tiles to request
    /// \return invalid if failed
    virtual ShaderDataMappingID CreateMapping(ShaderDataID data, uint64_t tileCount) = 0;

    /// Map a buffer
    /// \param rid resource id
    /// \return mapped buffer
    virtual void* Map(ShaderDataID rid) = 0;

    /// Flush a mapped range
    /// \param rid resource id
    /// \param offset byte offset
    /// \param length byte length from offset
    virtual void FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) = 0;

    /// Destroy an allocation
    /// \param rid allocation identifier
    virtual void Destroy(ShaderDataID rid) = 0;

    /// Destroy a mapping
    /// \param mid allocation identifier
    virtual void DestroyMapping(ShaderDataMappingID mid) = 0;

    /// Enumerate all created data
    /// \param count if [out] is null, filled with the number of resources
    /// \param out if not null, filled with all resources up to [count]
    virtual void Enumerate(uint32_t* count, ShaderDataInfo* out, ShaderDataTypeSet mask) = 0;
};
