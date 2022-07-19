#pragma once

// Layer
#include "DXILPhysicalBlockScan.h"
#include "Blocks/DXILPhysicalBlockFunction.h"
#include "Blocks/DXILPhysicalBlockType.h"
#include "Blocks/DXILPhysicalBlockGlobal.h"
#include "Blocks/DXILPhysicalBlockString.h"
#include "Blocks/DXILPhysicalBlockMetadata.h"
#include "Blocks/DXILPhysicalBlockSymbol.h"
#include "DXILIDMap.h"

// Std
#include <string>

// Forward declarations
struct DXJob;
struct DXStream;

struct DXILPhysicalBlockTable {
    DXILPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Parse the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const DXJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(DXStream &out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXILPhysicalBlockTable& out);

    /// Scanner
    DXILPhysicalBlockScan scan;

    /// Blocks
    DXILPhysicalBlockFunction function;
    DXILPhysicalBlockType type;
    DXILPhysicalBlockGlobal global;
    DXILPhysicalBlockString string;
    DXILPhysicalBlockSymbol symbol;
    DXILPhysicalBlockMetadata metadata;

    /// Shared identifier map
    DXILIDMap idMap;

    /// LLVM triple
    std::string triple;

    /// LLVM data layout
    std::string dataLayout;
};
