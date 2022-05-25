#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockGlobal.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockGlobal::ParseConstants(const struct LLVMBlock *block) {
    uint64_t type = 0;

    for (const LLVMRecord &record: block->records) {
        switch (static_cast<LLVMConstantRecord>(record.id)) {
            default: {
                ASSERT(false, "Unsupported constant record");
                return;
            }

            /* Set type for succeeding constants */
            case LLVMConstantRecord::SetType: {
                type = record.ops[0];
                break;
            }

            /* Just create the mapping for now */
            case LLVMConstantRecord::Null:
            case LLVMConstantRecord::Undef:
            case LLVMConstantRecord::Integer:
            case LLVMConstantRecord::Float:
            case LLVMConstantRecord::Aggregate:
            case LLVMConstantRecord::String:
            case LLVMConstantRecord::CString:
            case LLVMConstantRecord::Cast:
            case LLVMConstantRecord::GEP:
            case LLVMConstantRecord::InBoundsGEP:
            case LLVMConstantRecord::Data: {
                table.idMap.AllocMappedID(DXILIDType::Constant);
                break;
            }
        }
    }
}

void DXILPhysicalBlockGlobal::ParseGlobalVar(const struct LLVMRecord& record) {
    table.idMap.AllocMappedID(DXILIDType::Variable);
}

void DXILPhysicalBlockGlobal::ParseAlias(const LLVMRecord &record) {
    table.idMap.AllocMappedID(DXILIDType::Alias);
}
