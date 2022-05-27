#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockSymbol.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockSymbol::ParseSymTab(const LLVMBlock *block) {
    for (const LLVMRecord &record: block->records) {
        switch (static_cast<LLVMSymTabRecord>(record.id)) {
            default: {
                // Unexposed, not sure if danger or not...
                break;
            }
            case LLVMSymTabRecord::Entry: {
                /*
                 * LLVM Specification
                 *   VST_ENTRY: [valueid, namechar x N]
                 */

                // May not be mapped
                if (uint64_t value = record.Op(0); table.idMap.IsMapped(value)) {
                    // Grow to capacity
                    if (valueStrings.size() <= value) {
                        valueStrings.resize(value + 1);
                    }

                    // Debugging experience
#ifndef NDEBUG
                    char buffer[256];
                    if (record.opCount < 256) {
                        record.FillOperands(buffer, 1);
                    }
#endif // NDEBUG

                    // Insert from operand 1
                    valueStrings.push_back(LLVMRecordStringView(record, 1));
                }
                break;
            }
            case LLVMSymTabRecord::BasicBlockEntry: {
                /*
                 * LLVM Specification
                 *   VST_BBENTRY: [bbid, namechar x N]
                 */
                break;
            }
            case LLVMSymTabRecord::FunctionEntry: {
                /*
                 * LLVM Specification
                 *   VST_FNENTRY: [valueid, offset, namechar x N]
                 */
                break;
            }
            case LLVMSymTabRecord::CombinedEntry: {
                /*
                 * LLVM Specification
                 *   VST_COMBINED_ENTRY: [valueid, refguid]
                 */
                break;
            }
        }
    }
}

LLVMRecordStringView DXILPhysicalBlockSymbol::GetValueString(uint32_t id) {
    if (id >= valueStrings.size()) {
        return {};
    }

    return valueStrings[id];
}
