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
