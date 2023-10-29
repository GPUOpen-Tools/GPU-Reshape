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

// Layer
#include <Backends/DX12/DX12.h>
#include "DXBCPhysicalBlock.h"
#include "DXBCPhysicalBlockType.h"
#include "DXBCHeader.h"

// Common
#include <Common/Allocator/Vector.h>
#include <Common/Allocators.h>

// Forward declarations
struct DXStream;
struct DXCompileJob;

/// DXBC Scanner
struct DXBCPhysicalBlockScan {
public:
    DXBCPhysicalBlockScan(const Allocators& allocators);

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Scan(const void* byteCode, uint64_t byteLength);

    /// Stitch the resulting stream
    /// \param out
    bool Stitch(const DXCompileJob& job, DXStream& out, bool sign);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXBCPhysicalBlockScan& out);

    /// Get a physical block of type
    /// \return nullptr if not found
    DXBCPhysicalBlock* GetPhysicalBlock(DXBCPhysicalBlockType type) {
        for (Section& section : sections) {
            if (section.type == type) {
                return &section.block;
            }
        }

        // Not found
        return nullptr;
    }

    /// Add a new physical block
    /// \param type block type
    /// \return block
    DXBCPhysicalBlock& AddPhysicalBlock(DXBCPhysicalBlockType type) {
        ASSERT(!GetPhysicalBlock(type), "Duplicate physical block");

        // Create section
        Section& section = sections.emplace_back(allocators);
        section.type = type;
        section.unexposedType = static_cast<uint32_t>(type);
        return section.block;
    }

    /// Top header
    DXBCHeader header;

private:
    struct Section {
        Section(const Allocators& allocators) : block(allocators) {
            
        }
    
        uint32_t unexposedType;
        DXBCPhysicalBlockType type;
        DXBCPhysicalBlock block;
    };

    /// All sections
    Vector<Section> sections;

private:
    Allocators allocators;
};
