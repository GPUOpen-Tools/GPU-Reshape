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

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/Utils/DXILUtilCompliance.h>

DXILUtilCompliance::DXILUtilCompliance(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table):
    DXILUtil(allocators, program, table),
    declarations(allocators) {
    /* */
}

void DXILUtilCompliance::Compile() {
    LLVMBlock &root = table.scan.GetRoot();

    // Ensure the triple is as expected
    if (table.triple.empty()) {
        constexpr const char* kDefaultTriple = "dxil-ms-dx";

        // Setup record
        LLVMRecord record(LLVMModuleRecord::Triple);
        record.opCount = std::strlen(kDefaultTriple);
        record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);

        // Fill triple
        for (size_t i = 0; i < record.opCount; i++) {
            record.ops[i] = kDefaultTriple[i];
        }

        // Add it!
        root.AddRecord(record);
    }
}

void DXILUtilCompliance::TrimFunctions() {
    LLVMBlock &root = table.scan.GetRoot();

    // Only relevant for DXBC -> DXIL signing environments
    // Given that signing is currently disabled, so is this.
#if 1
    return;
#endif // 1

    // Linear function index
    uint32_t fnIndex = 0;

    // Trim all unused functions
    for (size_t i = 0; i < root.elements.size();) {
        const LLVMBlockElement& element = root.elements[i];

        // Not record?
        if (!element.Is(LLVMBlockElementType::Record)) {
            i++;
            continue;
        }

        // Not function?
        LLVMRecord& record = root.records[element.id];
        if (!record.Is(LLVMModuleRecord::Function)) {
            i++;
            continue;
        }

        // Get functionf rom index
        const DXILFunctionDeclaration *fn = table.function.GetFunctionDeclarationFromIndex(fnIndex);

        // If prototype, i.e. external, and not used, trim it
        if (fn->isPrototype && std::find(declarations.begin(), declarations.end(), fn) == declarations.end()) {
            root.elements.erase(root.elements.begin() + i);

            // Remove the associated symbol
            TrimSymbolForFunction(fnIndex);
        } else {
            i++;
        }

        // Next function
        fnIndex++;
    }
}

void DXILUtilCompliance::TrimSymbolForFunction(uint32_t index) {
    LLVMBlock* symTab = table.scan.GetRoot().GetBlock(LLVMReservedBlock::ValueSymTab);

    // Check all symbols
    for (size_t i = 0; i < symTab->elements.size(); i++) {
        const LLVMBlockElement& element = symTab->elements[i];

        // Must be entry record
        if (!element.Is(LLVMBlockElementType::Record) || !symTab->records[element.id].Is(LLVMSymTabRecord::Entry)) {
            continue;
        }

        const LLVMRecord& record = symTab->records[element.id];

        // Matched index?
        if (record.Op(0) != index) {
            continue;
        }

        // Erase current
        symTab->elements.erase(symTab->elements.begin() + i);
        break;
    }
}

void DXILUtilCompliance::CopyTo(DXILUtilCompliance &out) {
    out.declarations = declarations;
}

void DXILUtilCompliance::MarkAsUsed(const DXILFunctionDeclaration* anchor) {
    declarations.push_back(anchor);
}

