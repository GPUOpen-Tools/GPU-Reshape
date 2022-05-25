#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockString.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockString::ParseStrTab(const LLVMBlock* block) {
    // By specification, must have a single record
    if (block->records.Size() != 1) {
        ASSERT(false, "Unexpected record count");
        return;
    }

    // Get record
    blobRecord = block->records.Data();
}

std::string_view DXILPhysicalBlockString::GetString(uint64_t offset, uint64_t length) {
    if (!blobRecord) {
        return "";
    }

    // Get view
    ASSERT(offset + length <= blobRecord->blobSize, "StrTab substr out of bounds");
    return std::string_view(reinterpret_cast<const char*>(blobRecord->blob) + offset, length);
}
