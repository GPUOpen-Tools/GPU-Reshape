#pragma once

// Forward declarations
struct DXILPhysicalBlockTable;

/// Physical block
struct DXILPhysicalBlockSection {
    DXILPhysicalBlockSection(DXILPhysicalBlockTable& table) : table(table) {

    }

    /// Parent table
    DXILPhysicalBlockTable& table;
};
