#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockGlobal.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockGlobal::Parse() {
    LLVMBlock& block = table.scan.GetRoot();

    for (auto&& record : block.records) {
        switch (record.As<LLVMModuleRecord>()) {
            default:
                break;
            case LLVMModuleRecord::GlobalVar:
                break;
        }
    }
}
