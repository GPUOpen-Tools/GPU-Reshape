#pragma once

// Program
#include <Backend/IL/Program.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DXILPhysicalBlockTable;

/// Physical block
struct DXILPhysicalBlockSection {
    DXILPhysicalBlockSection(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table) :
        allocators(allocators), program(program), table(table) {
        /* */
    }

    /// Allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Block table
    DXILPhysicalBlockTable &table;
};
