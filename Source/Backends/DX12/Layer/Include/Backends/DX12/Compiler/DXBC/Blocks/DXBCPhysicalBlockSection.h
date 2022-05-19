#pragma once

// Program
#include <Backend/IL/Program.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DXBCPhysicalBlockTable;

/// Generic section
struct DXBCPhysicalBlockSection {
    DXBCPhysicalBlockSection(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table) :
        allocators(allocators), program(program), table(table) {
        /* */
    }

    /// Allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Parent table
    DXBCPhysicalBlockTable& table;
};
