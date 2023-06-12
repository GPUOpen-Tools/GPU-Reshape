#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"

// Std
#include <string_view>

// Forward declarations
struct DXParseJob;

/// Debug block
struct DXBCPhysicalBlockDebug : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockDebug(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

    /// Parse all instructions
    bool Parse(const DXParseJob& job);

private:
    /// Try to parse a PDB file
    /// \param path given path
    /// \return nullptr if failed
    DXBCPhysicalBlock* TryParsePDB(const std::string_view& path);

private:
    /// Scanner for external pdbs
    DXBCPhysicalBlockScan pdbScanner;

    /// Tie lifetime of external pdb to this block
    Vector<uint8_t> pdbContainerContents;
};
