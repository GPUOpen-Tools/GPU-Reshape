#pragma once

// Layer
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

/// Input signature block
struct DXBCPhysicalBlockInputSignature : public DXBCPhysicalBlockSection {
    using DXBCPhysicalBlockSection::DXBCPhysicalBlockSection;

    /// Parse signature
    void Parse();

    /// Compile signature
    void Compile();

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockInputSignature& out);

private:
    /// Signature header
    DXILInputSignature header;

    /// Givne element
    struct ElementEntry {
        /// Scanned signature
        DXILSignatureElement scan;

        /// Name string
        const char* semanticName{nullptr};
    };

    /// All entries
    std::vector<ElementEntry> entries;
};
