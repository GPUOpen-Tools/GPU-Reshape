#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockMetadata.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockMetadata::ParseMetadata(const struct LLVMBlock *block) {
    metadata.resize(block->records.Size());

    // Visit records
    for (size_t i = 0; i < block->records.Size(); i++) {
        const LLVMRecord &record = block->records[i];

        // Get metadata
        Metadata& md = metadata[i];

        // Handle record
        switch (static_cast<LLVMMetadataRecord>(record.id)) {
            default: {
                // Ignored
                break;
            }

            case LLVMMetadataRecord::Name: {
                // Validation
                ASSERT(i + 1 != block->records.Size(), "Expected succeeding metadata record");

                // Get next
                const LLVMRecord& next = block->records[++i];
                ASSERT(next.Is(LLVMMetadataRecord::NamedNode), "Succeeding record to Name must be NamedNode");
                break;
            }

            // String name?
            case LLVMMetadataRecord::StringOld: {
                md.name.resize(record.opCount);
                record.FillOperands(md.name.data());
                break;
            }
        }
    }
}
