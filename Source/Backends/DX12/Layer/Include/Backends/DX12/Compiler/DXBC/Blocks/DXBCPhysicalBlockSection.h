#pragma once

// Forward declarations
struct DXBCPhysicalBlockTable;

/// Generic section
struct DXBCPhysicalBlockSection {
    DXBCPhysicalBlockSection(DXBCPhysicalBlockTable& table) : table(table) {

    }

    /// Parent table
    DXBCPhysicalBlockTable& table;
};
