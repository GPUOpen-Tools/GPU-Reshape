// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockSymbol.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

DXILPhysicalBlockSymbol::DXILPhysicalBlockSymbol(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table)
    : DXILPhysicalBlockSection(allocators, program, table),
      valueStrings(allocators.Tag(kAllocModuleDXILSymbols)),
      valueAllocations(allocators.Tag(kAllocModuleDXILSymbols)),
      blockAllocator(allocators.Tag(kAllocModuleDXILSymbols)) {
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
                if (uint32_t value = record.Op32(0); table.idMap.IsMapped(value)) {
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
    for (size_t i = 0; i < block->elements.size(); i++) {
        const LLVMBlockElement& element = block->elements[i];
        
        if (!element.Is(LLVMBlockElementType::Record)) {
            continue;
        }
        
        LLVMRecord &record = block->records[block->elements[i].id];
        
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
