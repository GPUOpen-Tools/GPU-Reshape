#pragma once

// Program
#include <Backend/IL/Program.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;

/// Base physical block
struct SpvPhysicalBlockSection {
    SpvPhysicalBlockSection(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable &table) :
        allocators(allocators), program(program), table(table) {
        /* */
    }

    /// Destination block
    SpvPhysicalBlock *block{nullptr};

    /// Allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Block table
    SpvPhysicalBlockTable &table;
};
