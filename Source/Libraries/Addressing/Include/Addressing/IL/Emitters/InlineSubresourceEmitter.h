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

// Addressing
#include <Addressing/IL/PhysicalMipData.h>
#include <Addressing/IL/TexelCommon.h>
#include <Addressing/TexelMemoryDWordFields.h>

// Feature
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>

// Std
#include <cstdint>

template<typename E, typename RTE>
struct InlineSubresourceEmitter {
    /// Constructor
    /// \param emitter target emitter 
    /// \param tokenEmitter the resource token emitter
    /// \param buffer buffer with subresource information
    /// \param memoryBase header offset
    InlineSubresourceEmitter(IL::Emitter<E>& emitter, RTE& tokenEmitter, IL::ID buffer, IL::ID memoryBase) :
        emitter(emitter), tokenEmitter(tokenEmitter),
        buffer(buffer), memoryBase(memoryBase) {
        // Read the subresource count
        subresourceCount = ReadFieldDWord(TexelMemoryDWordFields::SubresourceCount);
    }

    /// Read a specific field dword
    /// \param field field to read
    /// \return dword
    IL::ID ReadFieldDWord(TexelMemoryDWordFields field) {
        IL::ID fieldId = emitter.GetProgram()->GetConstants().UInt(static_cast<uint32_t>(field))->id;

        // Read the field, header dwords always up first
        return emitter.Extract(
            emitter.LoadBuffer(buffer, emitter.Add(memoryBase, fieldId)),
            emitter.GetProgram()->GetConstants().UInt(0)->id
        );
    }

    /// Get the subresource offset starting offset
    /// \return offset
    IL::ID GetMemorySubresourceOffsetStart() {
        return emitter.Add(memoryBase, emitter.GetProgram()->GetConstants().UInt(static_cast<uint32_t>(TexelMemoryDWordFields::SubresourceOffsetStart))->id);
    }

    /// Get the memory base of the resource
    /// \return memory base
    IL::ID GetResourceMemoryBase() {
        return emitter.Add(GetMemorySubresourceOffsetStart(), subresourceCount);
    }

    /// Get the subresource offset of a slice major resource
    /// \param slice target slice
    /// \param mip target mip level
    /// \return offset data
    Backend::IL::PhysicalMipData<uint32_t> SlicedOffset(uint32_t slice, uint32_t mip) {
        IL::ExtendedEmitter extended(emitter);

        // Calculate the subresource index, mipCount * slice + mip
        IL::ID subresourceIndex = emitter.Add(emitter.Mul(tokenEmitter.GetMipCount(), slice), mip);

        // Load the subresource offset
        IL::ID subresourceMemoryIndex = emitter.Add(GetMemorySubresourceOffsetStart(), subresourceIndex);
        IL::ID subresourceOffset      = emitter.Extract(emitter.LoadBuffer(buffer, subresourceMemoryIndex), emitter.GetProgram()->GetConstants().UInt(0)->id);

        // Setup the mip data
        Backend::IL::PhysicalMipData<uint32_t> out;
        out.offset = subresourceOffset;
        out.mipWidth = Backend::IL::GetLogicalMipDimension(emitter, tokenEmitter.GetWidth(), mip);
        out.mipHeight = Backend::IL::GetLogicalMipDimension(emitter, tokenEmitter.GetHeight(), mip);
        return out;
    }

    /// Get the subresource offset of a mip major resource
    /// \param mip target mip level
    /// \return offset data
    Backend::IL::PhysicalMipData<uint32_t> VolumetricOffset(uint32_t mip) {
        IL::ExtendedEmitter extended(emitter);

        // Load the subresource offset
        IL::ID subresourceMemoryIndex = emitter.Add(GetMemorySubresourceOffsetStart(), mip);
        IL::ID subresourceOffset      = emitter.Extract(emitter.LoadBuffer(buffer, subresourceMemoryIndex), emitter.GetProgram()->GetConstants().UInt(0)->id);

        // Setup the mip data
        Backend::IL::PhysicalMipData<uint32_t> out;
        out.offset = subresourceOffset;
        out.mipWidth = Backend::IL::GetLogicalMipDimension(emitter, tokenEmitter.GetWidth(), mip);
        out.mipHeight = Backend::IL::GetLogicalMipDimension(emitter, tokenEmitter.GetHeight(), mip);
        out.mipDepth = Backend::IL::GetLogicalMipDimension(emitter, tokenEmitter.GetDepthOrSliceCount(), mip);
        return out;
    }

    /// Get the number of subresources
    IL::ID GetSubresourceCount() const {
        return subresourceCount;
    }

private:
    /// Target emitter
    IL::Emitter<E>& emitter;

    /// Resource token emitter
    RTE& tokenEmitter;

    /// Buffer containing subresource information
    IL::ID buffer;

    /// Base offset for resource data
    IL::ID memoryBase;

private:
    /// Total number of subresources
    IL::ID subresourceCount;
};
