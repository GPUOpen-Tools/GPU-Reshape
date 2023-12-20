// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>

SpvPhysicalBlockTable::SpvPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    program(program),
    scan(program),
    entryPoint(allocators, program, *this),
    capability(allocators, program, *this),
    annotation(allocators, program, *this),
    debugStringSource(allocators, program, *this),
    typeConstantVariable(allocators, program, *this),
    function(allocators, program, *this),
    shaderExport(allocators, program, *this),
    shaderPRMT(allocators, program, *this),
    shaderDescriptorConstantData(allocators, program, *this),
    shaderConstantData(allocators, program, *this) {
    /* */
}

bool SpvPhysicalBlockTable::Parse(const uint32_t *code, uint32_t count) {
    // Attempt to scan the blocks
    if (!scan.Scan(code, count)) {
        return false;
    }

    // Parse all blocks
    entryPoint.Parse();
    capability.Parse();
    annotation.Parse();
    debugStringSource.Parse();
    typeConstantVariable.Parse();
    function.Parse();

    // OK
    return true;
}

bool SpvPhysicalBlockTable::Compile(const SpvJob &job) {
    // Set the new bound
    scan.header.bound = program.GetIdentifierMap().GetMaxID();

    // Create id map
    SpvIdMap idMap;
    idMap.SetBound(&scan.header.bound);

    // Compile pre blocks
    typeConstantVariable.Compile(idMap);

    // Compile the shader export record
    shaderExport.CompileRecords(job);
    shaderDescriptorConstantData.CompileRecords(job);
    shaderConstantData.CompileRecords(job);
    shaderPRMT.CompileRecords(job);

    // Compile all functions
    if (!function.Compile(job, idMap)) {
        return false;
    }

    // Compile dynamic constant data
    typeConstantVariable.CompileConstants();

    // OK
    return true;
}

void SpvPhysicalBlockTable::Stitch(SpvStream &out) {
    // Write all blocks
    scan.Stitch(out);
}

void SpvPhysicalBlockTable::CopyTo(SpvPhysicalBlockTable &out) {
    scan.CopyTo(out.scan);
    capability.CopyTo(out, out.capability);
    annotation.CopyTo(out, out.annotation);
    debugStringSource.CopyTo(out, out.debugStringSource);
    typeConstantVariable.CopyTo(out, out.typeConstantVariable);
    function.CopyTo(out, out.function);
    shaderExport.CopyTo(out, out.shaderExport);
    shaderDescriptorConstantData.CopyTo(out, out.shaderDescriptorConstantData);
    shaderConstantData.CopyTo(out, out.shaderConstantData);
    shaderPRMT.CopyTo(out, out.shaderPRMT);
}
