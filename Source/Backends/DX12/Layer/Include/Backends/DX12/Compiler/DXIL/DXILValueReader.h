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
#include "DXILIDMap.h"
#include "DXILTypeMap.h"
#include "DXILPhysicalBlockTable.h"

struct DXILValueReader : public LLVMRecordReader {
    /// Constructor
    /// \param table instrumentation table
    /// \param record source record
    DXILValueReader(DXILPhysicalBlockTable& table, LLVMRecord& record) : LLVMRecordReader(record), table(table) {
        
    }

    /// Get a mapped relative value, may be forward
    /// \param anchor source anchor
    /// \return identifier
    uint32_t GetMappedRelative(uint32_t anchor) {
        uint32_t id = ConsumeOp32();

        // Fully resolved?
        if (table.idMap.IsResolved(anchor, id)) {
            return table.idMap.GetMappedRelative(anchor, id);
        } else {
            return table.idMap.GetMappedForward(anchor, DXILIDRemapper::DecodeForward(id));
        }
    }

    /// Get a mapped relative value, typed if forward
    /// \param anchor source anchor
    /// \return identifier
    uint32_t GetMappedRelativeValue(uint32_t anchor) {
        uint32_t id = ConsumeOp32();

        // Fully resolved?
        if (table.idMap.IsResolved(anchor, id)) {
            return table.idMap.GetMappedRelative(anchor, id);
        } else {
            // Create an unresolved value
            uint32_t forwardRelative = table.idMap.GetMappedForward(anchor, DXILIDRemapper::DecodeForward(id));

            // Immediate next is the type
            const Backend::IL::Type *resovledType = table.type.typeMap.GetType(ConsumeOp32());

            // Set the type, most future'd operations can get away with the type alone during parsing
            table.type.typeMap.GetProgramMap().SetType(forwardRelative, resovledType);

            // OK
            return forwardRelative;
        }
    }

private:
    DXILPhysicalBlockTable& table;
};
