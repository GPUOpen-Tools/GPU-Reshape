#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockSymbol.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockSymbol::DXILPhysicalBlockSymbol(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table) :
    DXILPhysicalBlockSection(allocators, program, table),
    blockAllocator(allocators) {
    /* */
}

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
                        valueAllocations.resize(value + 1);
                    }

                    // Insert from operand 1
                    valueStrings[value] = LLVMRecordStringView(record, 1);

                    // Debugging experience
#ifndef NDEBUG
                    char buffer[256];
                    if (record.opCount < 256) {
                        record.FillOperands(buffer, 1);
                    }

                    GetValueAllocation(value);
#endif // NDEBUG
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

const char* DXILPhysicalBlockSymbol::GetValueAllocation(uint32_t id) {
    if (id >= valueStrings.size() || !valueStrings[id]) {
        return nullptr;
    }

    // Current view
    const LLVMRecordStringView& view = valueStrings[id];

    // Not allocated?
    if (!valueAllocations[id]) {
        valueAllocations[id] = blockAllocator.AllocateArray<char>(view.Length() + 1);
        view.CopyTerminated(valueAllocations[id]);
    }

    return valueAllocations[id];
}

void DXILPhysicalBlockSymbol::CompileSymTab(struct LLVMBlock *block) {

}

void DXILPhysicalBlockSymbol::StitchSymTab(struct LLVMBlock *block) {
    for (LLVMRecord &record: block->records) {
        switch (static_cast<LLVMSymTabRecord>(record.id)) {
            default: {
                // Ignored
                break;
            }

            case LLVMSymTabRecord::Entry: {
                table.idRemapper.Remap(record.Op(0));
                break;
            }
        }
    }
}

void DXILPhysicalBlockSymbol::CopyTo(DXILPhysicalBlockSymbol &out) {
    out.valueAllocations = valueAllocations;
    out.valueStrings = valueStrings;
}
