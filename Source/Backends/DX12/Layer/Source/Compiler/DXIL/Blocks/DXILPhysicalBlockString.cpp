// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockString.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 */

void DXILPhysicalBlockString::ParseStrTab(const LLVMBlock* block) {
    // By specification, must have a single record
    if (block->records.size() != 1) {
        ASSERT(false, "Unexpected record count");
        return;
    }

    // Get record
    blobRecord = block->records.data();
}

std::string_view DXILPhysicalBlockString::GetString(uint64_t offset, uint64_t length) {
    if (!blobRecord) {
        return "";
    }

    // Get view
    ASSERT(offset + length <= blobRecord->blobSize, "StrTab substr out of bounds");
    return std::string_view(reinterpret_cast<const char*>(blobRecord->blob) + offset, length);
}

void DXILPhysicalBlockString::CompileStrTab(struct LLVMBlock *block) {

}

void DXILPhysicalBlockString::StitchStrTab(struct LLVMBlock *block) {

}
