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

// Layer
#include "DXILIDMap.h"
#include "DXILTypeMap.h"
#include "DXILPhysicalBlockTable.h"

// Common
#include <Common/Sink.h>

struct DXILValueWriter {
    /// Constructor
    /// \param table instrumentation table
    /// \param record source record
    DXILValueWriter(DXILPhysicalBlockTable& table, LLVMRecord& record) : table(table), record(record), destOperands(record.ops), destLength(record.opCount) {
        
    }

    /// Remap a relative value
    /// \param anchor the stitch anchor
    void RemapRelative(DXILIDRemapper::Anchor& anchor) {
        uint64_t id = record.Op(sourceOffset++);

        // Allow forward stitching
        const bool isForward = table.idRemapper.RemapRelative(anchor, record, id);
        GRS_SINK(isForward);

        destOperands[destOffset++] = id;
    }

    /// Remap a relative value, typed if forward
    /// \param anchor
    void RemapRelativeValue(DXILIDRemapper::Anchor& anchor) {
        uint64_t id = record.Op(sourceOffset++);

        // Remap separately to keep id valid
        uint64_t remapped = id;

        // Allow forward stitching
        const bool isForward = table.idRemapper.RemapRelative(anchor, record, remapped);
        destOperands[destOffset++] = remapped;
        
        // User records never have forward types
        if (record.userRecord) {
            if (isForward) {
                Migrate();

                // User to IL id
                uint32_t value = DXILIDRemapper::DecodeUserOperand(id);

                // Store forward type
                destOperands[destOffset++] = table.type.typeMap.GetType(table.program.GetTypeMap().GetType(value)); 
            }
        } else {
            // Was the source record forward referenced?
            const bool forwardSource = id > record.sourceAnchor;

            // If the forward referencing has changed, re-emit
            if (forwardSource != isForward) {
                Migrate();

                // Promotion or demotion?
                if (forwardSource) {
                    // Ignore the type value
                    sourceOffset++;
                } else {
                    // Get the originating identifier
                    uint32_t value = table.idMap.GetMappedRelative(record.sourceAnchor, static_cast<uint32_t>(id));

                    // Store forward type
                    destOperands[destOffset++] = table.type.typeMap.GetType(table.program.GetTypeMap().GetType(value));
                }
            }
        }
    }

    /// Skip a number of operands
    /// \param count operands to skip
    void Skip(uint32_t count) {
        // Skip copy if no split
        if (destOperands != record.ops) {
            std::memcpy(destOperands + destOffset, record.ops + sourceOffset, sizeof(uint64_t) * count);
        }

        // Offset both
        sourceOffset += count;
        destOffset += count;
    } 

    /// Migrate all source operands
    void Migrate() {
        // Copy with +1 length
        auto* ops = table.recordAllocator.AllocateArray<uint64_t>(static_cast<uint32_t>(++destLength));
        std::memcpy(ops, destOperands, sizeof(uint64_t) * sourceOffset);
        destOperands = ops;

        // The source record may not be user added, in which case the original abbreviation is active and should be invalidated
        record.abbreviation.type = LLVMRecordAbbreviationType::None;
    }

    /// Finalize all world
    void Finalize() {
        // No work to do if the same
        if (destOperands == record.ops) {
            return;
        }

        // Missing some operands til end?
        uint32_t missing =  record.opCount - sourceOffset;

        // Copy missing
        ASSERT(destOffset + missing <= destLength, "Out of bounds operand writing");
        std::memcpy(destOperands + destOffset, record.ops + sourceOffset, sizeof(uint64_t) * missing);

        // Replace source operands
        record.ops = destOperands;
        record.opCount = destOffset + missing;
    }

private:
    DXILPhysicalBlockTable& table;

    /// Source record
    LLVMRecord& record;

    /// Destination operands, to be placed into the source record
    uint64_t* destOperands{nullptr};

    /// Number of destination operands
    uint64_t destLength{0};

    /// Current source consumption offset
    uint32_t sourceOffset{0};

    /// Current destination writing offset
    uint32_t destOffset{0};
};
